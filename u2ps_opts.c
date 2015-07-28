#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u2ps.h"
#include "warn.h"
#include "u2ps_data.h"

char* inputname = NULL;
char* outputname = NULL;
char* tmpoutname = NULL;

char** passopts = { NULL };
int passnum = 0;

int verbose = 0;

struct genopts genopts = {
	.startline = 1,
	.landscape = NO,
	.cols = 0,
	.rows = 0,
	.tabstop = 8
};

struct runopts runopts;

struct pagelayout pagelayout = {
	.pw = 595, .ph = 842,		/* A4 */
	.mt = 55, .ml = 57.5,		/* some reasonable margins */
	.mb = 55, .mr = 57.5
};

struct fonts fonts = {
	.head = { "Times-Roman" },
	.line = { "Times-Roman" },
	.text = {
		[REGULAR] = { "FreeMono", 12 }
	}
};

struct headings headings = {
	.hl = FILENAME,
	.hr = PAGENO
};

static void die_print_usage(void);
static int handle_longopt(int argc, char** argv, int i);
static int handle_shortopt(int argc, char** argv, int i);
static void set_derivative_parameters(void);

void handle_args(int argc, char** argv)
{
	int i = 1;
	int (*handler)(int argc, char** argv, int i);

	if(argc < 2)
		die_print_usage();

	passopts = malloc(argc*sizeof(char*));

	while(i < argc) {
		if(argv[i][0] != '-')
			break;
		else if(!argv[i][1])
			{ i++; goto LO; }
		else if(argv[i][1] != '-')
			handler = handle_shortopt;
		else if(!argv[i][2])
			{ i++; goto LI; }
		else
			handler = handle_longopt;

		i += handler(argc, argv, i) ? 2 : 1;
	}

LI:	if(i < argc)
		inputname = argv[i++];
LO:	if(i < argc)
		outputname = argv[i++];
	if(i < argc)
		die("Too many arguments\n");

	if(runopts.stdout && !inputname)
		die("Either use - or specify input file name with -o\n");
	if(runopts.stdout && outputname)
		die("Using -o with output file name makes no sense\n");

	set_derivative_parameters();
}

void die_print_usage(void)
{
	printf("Usage: u2ps [options] input [output]\n");
	exit(0);
}

#define BOOL 0
#define INT  1
#define STRING 2
#define FUNC 3
#define PASS 4

static void set_paper(char* opt);
static void set_margins(char* opt);
static void set_empty_headings(void);
static void add_passopt(char* opt);

struct option {
	short shortopt;
	char* longopt;
	short type;
	void* addr;
} optlist[] = {
	{ 'T', "tab",		INT,	&genopts.tabstop },
	{ 'a', "noansi",	BOOL,	&genopts.skipansi },
	{ 'P', "paper",		FUNC,	set_paper },
	{ 'M', "margins",	FUNC,	set_margins },
	{ 'r', "landscape",	BOOL,	&genopts.landscape },
	{ 'C', "columns",	INT,	&genopts.cols },
	{ 'L', "lines",		INT,	&genopts.rows },
	{ '-', "head-font",	STRING, &fonts.head.name },
	{ '-', "line-font",	STRING, &fonts.line.name },
	{ 'o', "stdout",	BOOL,	&runopts.stdout },
	{ 'w', "wrap",		BOOL,	&genopts.wrap },
	{ 'm', "mark",		BOOL,	&genopts.mark },
	{ 'i', "inverse", 	BOOL,	&genopts.inverse },
	{ 'l', "numbers",	BOOL,	&genopts.linenum },
	{ '-', "startline",	INT,	&genopts.startline },
	{ 'b', "bookish",	BOOL,	&genopts.bookish },
	{ 't', "title", 	STRING,	&genopts.title },
	{ 'H', "noheadings", 	FUNC,	set_empty_headings },
	{ 'R', "noreduce",	BOOL,	&runopts.noreduce },
	{ 'A', "allfonts",	BOOL,	&runopts.allfonts },
	{ 'E', "noembed",	BOOL,	&runopts.skipfrem },
	{ 'k', "keep",		BOOL,	&runopts.keeptemp },
	{ 'd', NULL, 		PASS,	add_passopt },
	{ 'I', NULL,		PASS,	add_passopt },
	{ 'v', "verbose",	BOOL,	&verbose },
	{  0 }
};

#define GOT_THIS_OPT_ONLY 0
#define GOT_NEXT_ARG_TOO 1

static void handle_opt(struct option* opt, char* arg);

int handle_longopt(int argc, char** argv, int i)
{
	struct option* opt;
	char* arg = NULL;
	int ret = GOT_THIS_OPT_ONLY;

	if((arg = strpbrk(argv[i], "=")))
		*(arg++) = '\0';

	for(opt = optlist; opt->shortopt; opt++)
		if(!strcmp(opt->longopt, argv[i] + 2))
			break;
	if(!opt->shortopt)
		die("Unknown option %s\n", argv[i]);

	if(opt->type != BOOL && !arg) {
		if(++i >= argc)
			die("Argument required for %s\n", argv[i]);
		arg = argv[i];
		ret = GOT_NEXT_ARG_TOO;
	}

	handle_opt(opt, arg);

	return ret;
}

int handle_shortopt(int argc, char** argv, int i)
{
	struct option* opt;
	char* arg = NULL;
	int ret = GOT_THIS_OPT_ONLY;
	char* p = argv[i] + 1;

	do {
		for(opt = optlist; opt->shortopt; opt++)
			if(*p == opt->shortopt)
				break;
		if(!opt->shortopt)
			die("Unknown option -%c\n", *p);

		if(opt->type == PASS) {
			if(p > argv[i] + 1)
				die("Cannot mix -%c with other options in %s\n", *p, argv[i]);
			if(!*(p+1))
				die("Non-spaced argument required for -%c\n", *p);
			arg = argv[i];
		} else if(opt->type != BOOL) {
			if(*(p + 1)) {
				arg = p + 1;
			} else if(++i < argc) {
				arg = argv[i];
				ret = GOT_NEXT_ARG_TOO;
			} else {
				die("Argument required for -%c\n", *p);
			}
		}

		handle_opt(opt, arg);

	} while(opt->type == BOOL && *(++p));

	return ret;
}

void handle_opt(struct option* opt, char* arg)
{
	switch(opt->type) {
		case BOOL:
			*((bool*) opt->addr) = 1;
			break;
		case INT:
			*((int*) opt->addr) = atoi(arg);
			break;
		case STRING:
			*((char**) opt->addr) = arg;
			break;
		case PASS:
		case FUNC:
			((void (*)(char*)) opt->addr)(arg);
			break;
	}
}

/* Paper size is either a name like "a4", or numeric W:H dimensions. */

void set_paper(char* opt)
{
	char* sep = strpbrk(opt, ":");

	if(sep) {
		*sep++ = '\0';
		pagelayout.pw = atoi(opt);
		pagelayout.ph = atoi(sep);
	} else {
		const struct papersize* p;
		for(p = papersize; p->name; p++)
			if(!strcmp(p->name, opt))
				break;
		if(!p->name)
			die("Unknown paper size %s\n", opt);

		pagelayout.pw = p->pw;	
		pagelayout.ph = p->ph;
	}
}

/* Colon-separated CSS-style margins. */

void set_margins(char* opt)
{
	char* l1 = strsep(&opt, ":");
	char* l2 = strsep(&opt, ":");
	char* l3 = strsep(&opt, ":");
	char* l4 = opt;

	pagelayout.mt = l1 ? atoi(l1) : 0;
	pagelayout.mr = l2 ? atoi(l2) : pagelayout.mt;
	pagelayout.mb = l3 ? atoi(l3) : pagelayout.mt;
	pagelayout.ml = l4 ? atoi(l4) : pagelayout.mr; 
}

void set_empty_headings(void)
{
	memset(&headings, 0, sizeof(headings));
}

void add_passopt(char* opt)
{
	/* passopts is argc-long, and at most all of argv[1:] can be
	   pushed there, so there is no need for bound checks here. */
	passopts[passnum++] = opt;
	passopts[passnum] = NULL;
}

/* Font and terminal size settings are heavily interconnected: we need
   to know primary font aspect ratio before deciding terminal size,
   terminal size may affect (set) primary font size, which in turn affects
   sizes of all other fonts. Because of this, fonts are set up in two steps,
   with terminal size selection between them. */

static void set_font_aspects(void);
static void set_font_variants(void);
static void set_font_sizes(void);
static void set_termfontsize(int tbw, int tbh);

void set_derivative_parameters()
{
	int tbw = pagelayout.pw - pagelayout.ml - pagelayout.mr;
	int tbh = pagelayout.ph - pagelayout.mt - pagelayout.mb;

	set_font_aspects();
	set_font_variants();

	if(genopts.landscape)
		set_termfontsize(tbh, tbw);	
	else
		set_termfontsize(tbw, tbh);

	set_font_sizes();

	if(headings.hl || headings.hc || headings.hr
	|| headings.fl || headings.fc || headings.fr)
		headings.any = YES;

	if(genopts.mark)
		genopts.wrap = YES;
}

/* The following tries to set some consistent values for the primary
   font size, the number of lines and the number of columns for wrapping.
   Any of the three can be set by the user, and by this point bounding box
   (paper size minus margins) is known as well. */

void set_termfontsize(int tw, int th)
{
	int fs = 100*fonts.basesize;
	int fa = fonts.aspect;

	int cols = genopts.cols;
	int rows = genopts.rows;

	if(!fs && !cols && !rows)
		fs = 1200;

	int fsc = cols ? 100*tw/cols : 0;
	int fsr = rows ? 100*th/rows : 0;

	if(fsc && fsc < fsr) fsr = 0;
	if(fsr && fsr < fsc) fsc = 0;

	if(fsr) fs = fsr;
	if(fsc) fs = fsc;

	genopts.cols = 100*tw/fs*1000/fa;
	genopts.rows = 100*th/fs;
	fonts.basesize = fs/100;

	if(!fonts.head.size)
		fonts.head.size = 8*fs/1000;
	if(!fonts.line.size)
		fonts.line.size = 8*fs/1000;

	if(verbose)
		warn("Terminal area: %ix%ipt, font size %ipt, %i cols %i rows\n",
			tw, th, fonts.basesize, genopts.cols, genopts.rows);
}

void set_font_aspects(void)
{
	int i;
	struct font* fn = fonts.text;
	const struct fontaspect* a;

	for(i = 0; i < nFONTS; i++) {
		if(!fn[i].name || fn[i].aspect)
			continue;
		for(a = fontaspects; a->name; a++)
			if(!strcmp(a->name, fn[i].name))
				fn[i].aspect = a->aspect;
	}

	fonts.aspect = fonts.text[0].aspect;
	if(!fonts.aspect)
		fonts.aspect = 500;
}

#define count(a) (sizeof(a)/sizeof(*a))

void set_font_variants(void)
{
	int i;
	struct font* fn = fonts.text;
	const struct fontvariant* v;
	int variants[] = { BOLD, ITALIC, BOLDITALIC };

	for(i = 0; i < count(variants); i++)
		if(fn[variants[i]].name)
			return;

	for(v = fontvariants; v->base; v++)
		if(!strcmp(v->base, fn[REGULAR].name))
			break;
	if(!v->base)
		return;

	for(i = 0; i < count(variants); i++) {
		fn[variants[i]].name = v->font[i];
		fn[variants[i]].aspect = fn[0].aspect;
	}
}

void set_font_sizes(void)
{
	int i;
	struct font* fn = fonts.text;

	fn[0].size = fonts.basesize;

	for(i = 1; i < nFONTS; i++) {
		if(!fn[i].name)
			continue;
		if(!fn[i].size)
			fn[i].size = fn[0].size;
		if(!fn[i].aspect)
			fn[i].aspect = fn[0].aspect;

		fn[i].xscale = 1000*fn[0].aspect/fn[i].aspect;
	}
}
