#define NONE		0
#define STANDARD45	1
#define NONSTANDARD	2

#define NO		0
#define YES		1

#define FILENAME ((void*) 1)
#define PAGENO   ((void*) 2)

typedef unsigned char bool;

/* All options are located and initialized in u2ps_opts.c */

extern char* inputname;
extern char* outputname;
extern char* tmpoutname;

extern char** passopts;
extern int passnum;

extern int verbose;

extern struct genopts {
	int startline;
	bool landscape;
	bool skipansi;
	bool wrap;
	bool mark;
	bool inverse;
	bool linenum;
	bool bookish;

	char* title;

	int cols;
	int rows;
	int tabstop;
} genopts;

extern struct headings {
	bool any;
	char *hl, *hc, *hr;	/* header/footer left/center/right */
	char *fl, *fc, *fr;
} headings;

extern struct runopts {
	bool skipfonts;
	bool embedstdfonts;
	bool skipnotdef;
	bool skipfrem;
	bool stdout;
	bool skipunlink;
} runopts;

extern struct pagelayout {
	int pw, ph;		/* paper size, non-rotated even in landscape mode */
	int mt, mb, ml, mr;
} pagelayout;

struct font {
	int size;
	char* name;
};

extern struct fonts {
	struct font text;
	struct font head;
	struct font line;
} fonts;

/* u2ps_opts.c */
void handle_args(int argc, char** argv);

/* u2ps_term.c */
int print_chunk(char* chunk, int softlen);

/* u2ps_tset.c */
void put_global_setup(void);

/* u2ps_tcsi.c */
void new_page_attr(void);
void new_line_attr(void);
void end_line_attr(void);
void handle_csi(int cmd, int argc, int* args);

/* u2ps_unicode.c */
int deutf(unsigned char* ptr, int* codepoint);
int uniwidth(int codepoint);

/* u2ps_pswr.c */
void psnl(int blanks);
void psline(const char* line, ...);
void pscmd(const char* command, ...);
void psuni(const char* string, int len);
void psbad(int len);
void psstr(const char* string);
