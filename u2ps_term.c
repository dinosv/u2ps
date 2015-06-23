#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "u2ps.h"
#include "warn.h"

extern char* inputname;
extern struct genopts genopts;
extern struct pagelayout pagelayout;
extern struct headings headings;
extern struct fonts fonts;

bool activeline = 0;
int page = 0;
int hardline = 0;
int softline = 0;
int softcol = 0;

static int take_esc(char* ptr);
static int take_ctl(char* ptr);
static int take_uni(char* ptr);
static int take_csi(char* ptr);

enum linebreak { HARD = 0, SOFT = 1 };
void new_page(void);
void end_page(void);
void new_line(enum linebreak soft);

extern void psnl(int blanks);
extern void psline(const char* line, ...);
extern void pscmd(const char* command, ...);
extern void psuni(const char* string, int len);
extern void psbad(int len);
extern void psstr(const char* string);

extern int deutf(char* ptr, int* codepoint);
extern int uniwidth(int codepoint);

extern void put_global_setup(void);
extern void put_font_color(void);
static void put_header(const char* cmd, const char* string);

extern void new_page_font_color(void);
extern void handle_csi(int cmd, int argc, int* args);

int print_chunk(char* chunk, int start, int softlen)
{
	char* ptr = chunk;
	char* end = chunk + softlen;
	int adv;

	while(ptr < end) {
		if(!activeline)
			new_line(HARD);
		if(*ptr == 0x1B)
			adv = take_esc(ptr);
		else if(*ptr >= 0x00 && *ptr < 0x20)
			adv = take_ctl(ptr);
		else
			adv = take_uni(ptr);
		if(!adv)
			break;
		else
			ptr += adv;
	}

	return ptr - chunk;
}

int take_ctl(char* ptr)
{
	switch(*ptr)
	{
		case 0x0A: /* newline */
			pscmd("n"); psnl(0);
			activeline = 0;
			break;
		case 0x0D: /* carriage return */
			pscmd("cr"); psnl(0);
			softcol = 0;
			break;
		case 0x09: /* tab */
			pscmd("t");
			softcol += (genopts.tabstop - softcol % genopts.tabstop);
			break;
		case 0x08: /* backspace */
			if(softcol <= 0) break;
			pscmd("bs");
			softcol--;
			break;
	}
	return 1;
}

int take_esc(char* ptr)
{
	switch(*(ptr+1)) {
		case '\0': return 1;
		case '[': return take_csi(ptr);
		default: return 2;
	}
}

#define MAXCSI 8

int take_csi(char* ptr)
{
	int cmd = 0;
	char* p = ptr + 2;
	int argi = 0;
	int args[MAXCSI];

	memset(args, 0, sizeof(args));

	if(*p == '?') {
		p++;
		cmd |= 0x100;
	}

	for(; *p; p++) {
		if(*p >= '0' && *p <= '9') {
			if(argi < MAXCSI)
				args[argi] = 10*args[argi] + (*p - '0');
		} else if(*p == ';') {
			if(argi < MAXCSI)
				argi++;
		} else {
			cmd |= *p;
			break;
		}
	}

	handle_csi(cmd, argi + 1, args);

	return p - ptr + 1;
}

int take_uni(char* ptr)
{
	int codepoint;
	int len = deutf(ptr, &codepoint);
	int width = len < 0 ? 1 : uniwidth(codepoint);

	if(genopts.cols && genopts.wrap && softcol + width > genopts.cols) {
		pscmd("w"); psnl(0);
		new_line(SOFT);
	}
	softcol += width;

	if(len < 0)
		psbad(len = -len);
	else
		psuni(ptr, len);

	return len;
}

void new_file(void)
{
	time_t now = time(NULL);

	psline("%%!PS-Adobe-2.0\n");
	psline("%%%%BoundingBox: 0 0 %i %i\n", pagelayout.pw, pagelayout.ph);
	psline("%%%%Orientation: %s\n", genopts.landscape ? "Landscape" : "Portrait");
	if(genopts.title)
		psline("%%%%Title: %s\n", genopts.title);
	psline("%%%%Pages: (atend)\n");
	psline("%%%%Creator: u2ps\n");
	psline("%%%%CreationDate: %s\n", ctime(&now));
	psline("%%%%EndComments\n");
	psline("\n");

	psline("%%%%BeginProlog\n");
	psline("%%%%IncludeResource: font notdef\n");
	psline("%%%%IncludeResource: procset fontset\n");
	psline("%%%%IncludeResource: procset uniterm\n");
	psline("%%%%IncludeResource: fontset %s\n", fonts.text.name);
	if(strcmp(fonts.head.name, fonts.text.name))
		psline("%%%%IncludeResource: fontset %s\n", fonts.head.name);
	if(strcmp(fonts.line.name, fonts.text.name) && strcmp(fonts.line.name, fonts.head.name))
		psline("%%%%IncludeResource: fontset %s\n", fonts.line.name);
	psline("%%%%IncludeResource: fontset Times-Roman\n");
	psline("%%%%EndProlog\n");
	psnl(1);
	
	put_global_setup();

	hardline = genopts.startline;
}

void end_file(void)
{
	end_page();
	psline("%%%%Trailer\n");
	psline("%%%%Pages: %i\n", page);
	psline("%%%%EOF\n");
}

void new_line(enum linebreak soft)
{
	if(softline % genopts.rows == 0)
		new_page();

	if(genopts.linenum && !soft)
		pscmd("%i l", hardline);

	activeline = 1;
	softline++;
	if(!soft) hardline++;
	softcol = 0;
}

void new_page(void)
{
	if(page) end_page();

	page++;
	softcol = 0;
	softline = 0;

	psline("%%%%Page: %i %i\n", page, page);
	if(genopts.inverse)
		pscmd("save");
	if(genopts.landscape)
		pscmd("la");
	if(genopts.inverse)
		pscmd("bk");

	int mirror = (genopts.bookish && (page % 2));

	put_header("Hc", headings.hc);
	put_header("Hl", mirror ? headings.hr : headings.hl);
	put_header("Hr", mirror ? headings.hl : headings.hr);

	put_header("Fc", headings.fc);
	put_header("Fl", mirror ? headings.fr : headings.fl);
	put_header("Fr", mirror ? headings.fl : headings.fr);

	pscmd("tr");
	new_page_font_color();
	psnl(1);
}

void end_page(void)
{
	psnl(1);
	if(softline)
		pscmd("showpage");
	if(softline && genopts.inverse)
		pscmd("restore");
	psnl(1);
}

void put_header(const char* cmd, const char* string)
{
	if(string == PAGENO) {
		pscmd("(%i)%s", page, cmd); 
		return;
	};

	if(string == FILENAME)
		string = inputname;
	if(!string)
		return;

	psstr(string);
	pscmd(cmd);
}