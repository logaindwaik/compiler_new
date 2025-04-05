#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define BSIZE 128    /* buffer size */
#define NONE -1
#define EOS '\0'

/* تعريف التوكنز */
#define NUM 256
#define DIV 257
#define MOD 258
#define ID 259
#define DONE 260
#define PROGRAM 261
#define INPUT 262
#define OUTPUT 263


extern int tokenval;              
extern int lineno;
extern void factor();
extern void expr();
extern void term();
extern void match(int t);
extern void error(char* m);
extern void init();               
extern int lexan();               
extern void parse();              
extern int insert(char *s, int tok);  
extern int lookup(char *s);       
extern void emit(int t, int tval);

FILE *inputFile;      
FILE *outputFile;     


struct entry {
    char *lexptr;
    int token;
};

struct entry symtable[100];

/*********** lexer.c ***********/
char lexbuf[BSIZE];
int lineno = 1;
int tokenval = NONE;

int lexan() {
    int t;
    while (1) {
        t = fgetc(inputFile);  
        if (t == ' ' || t == '\t')
            ;  
        else if (t == '\n')
            lineno++;
        else if (t == '#') {  
            while ((t = fgetc(inputFile)) != '\n' && t != EOF);
            if (t == '\n')
                lineno++;
        }
        else if (isdigit(t)) {  
            ungetc(t, inputFile);
            fscanf(inputFile, "%d", &tokenval);
            return NUM;
        }
        else if (isalpha(t)) {  
            int p, b = 0;
            while (isalnum(t)) {
                lexbuf[b++] = t;
                t = fgetc(inputFile);
                if (b >= BSIZE)
                    error("compiler error: token too long");
            }
            lexbuf[b] = EOS;
            if (t != EOF)
                ungetc(t, inputFile);
            p = lookup(lexbuf);
            if (p == 0)
                p = insert(lexbuf, ID);
            tokenval = p;
            return symtable[p].token;
        }
        else if (t == EOF)
            return DONE;
        else {
            tokenval = NONE;
            return t;  
        }
    }
}

/*********** parser.c ***********/
int lookahead;
void parse() {
    
    lookahead = lexan();
    match(PROGRAM);
    match(ID);
    match('(');
    match(INPUT);
    match(',');
    match(OUTPUT);
    match(')');
    match('{');
    fprintf(outputFile, "program test(input,output)\n{\n");  
    while (lookahead != '}') {
        expr();
        match(';');  
        fprintf(outputFile, ";\n");  
    }
    match('}');
    fprintf(outputFile, "}\n");  
}

void expr() {
    int t;
    term();
    while (1) {
        switch (lookahead) {
            case '+': case '-':
                t = lookahead;
                match(t);
                term();
                emit(t, NONE);
                continue;
            default:
                return;
        }
    }
}

void term() {
    int t;
    factor();
    while (1) {
        switch (lookahead) {
            
            case '*': case '/': case DIV: case MOD: case '%':
                t = lookahead;
                match(t);
                factor();
                emit(t, NONE);
                continue;
            default:
                return;
        }
    }
}

void factor() {
    switch (lookahead) {
        case '(':
            match('(');
            expr();
            match(')');
            break;
        case NUM:
            emit(NUM, tokenval);
            match(NUM);
            break;
        case ID:
            emit(ID, tokenval);
            match(ID);
            break;
        default:
            error("syntax error in factor");
    }
}

void match(int t) {
    if (lookahead == t)
        lookahead = lexan();
    else {
        fprintf(stderr, "Expected token %d, but found token %d\n", t, lookahead);
        error("syntax error in match");
    }
}

/*********** emitter.c ***********/
void emit(int t, int tval) {
    switch (t) {
        case '+': case '-': case '*': case '/':
            fprintf(outputFile, "%c ", t);
            break;
        case '%':
            fprintf(outputFile, "%% ");
            break;
        case DIV:
            fprintf(outputFile, "DIV ");
            break;
        case MOD:
            fprintf(outputFile, "MOD ");
            break;
        case NUM:
            fprintf(outputFile, "%d ", tval);
            break;
        case ID:
            fprintf(outputFile, "%s ", symtable[tval].lexptr);
            break;
        default:
            fprintf(outputFile, "token %d, tokenval %d ", t, tval);
    }
}

/*********** symbol.c ***********/
#define STRMAX 999
#define SYMMAX 100
char lexemes[STRMAX];
int lastchar = -1;
int lastentry = 0;

int lookup(char s[]) {
    int p;
    for (p = lastentry; p > 0; p--)
        if (strcmp(symtable[p].lexptr, s) == 0)
            return p;
    return 0;
}

int insert(char s[], int tok) {
    int len = strlen(s);
    if (lastentry + 1 >= SYMMAX)
        error("symbol table full");
    if (lastchar + len + 1 >= STRMAX)
        error("lexemes array full");
    lastentry++;
    symtable[lastentry].token = tok;
    symtable[lastentry].lexptr = &lexemes[lastchar + 1];
    lastchar += len + 1;
    strcpy(symtable[lastentry].lexptr, s);
    return lastentry;
}

/*********** init.c ***********/
struct entry keywords[] = {
    {"div", DIV},
    {"mod", MOD},
    {"program", PROGRAM},
    {"input", INPUT},
    {"output", OUTPUT},
    {NULL, 0}
};

void init() {
    for (struct entry *p = keywords; p->token; p++) {
        insert(p->lexptr, p->token);
    }
}

/*********** error.c ***********/
void error(char *m) {
    fprintf(stderr, "line %d: %s\n", lineno, m);
    exit(1);
}

/*********** main.c ***********/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(1);
    }
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        fprintf(stderr, "Error opening input file %s\n", argv[1]);
        exit(1);
    }
    outputFile = fopen(argv[2], "w");
    if (!outputFile) {
        fprintf(stderr, "Error opening output file %s\n", argv[2]);
        exit(1);
    }
    init();
    parse();
    fclose(inputFile);
    fclose(outputFile);
    return 0;
}
