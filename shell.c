#include <unistd.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct simple_comm {
	char*  com;
	char* arg;} simple_comm; // простая команда

typedef struct conv {
	struct conv* next;
	simple_comm* com;} conv; // конвейер

typedef struct comm {
    int append_output;
	char* input;
	char* output;
	conv* convey;} comm; // команда

typedef struct cond_comm {
	struct cond_comm* next;
	char cond;
	comm* com;} cond_comm; // команда с условным выполнением

typedef struct shell_comm {
	cond_comm* cond_com;
	struct shell_comm* shell_com;
	struct shell_comm* next;
	char cond;
	int bg;
    int br;} shell_comm; // команда shell

simple_comm* init_simple_comm() {
	simple_comm* a = malloc(sizeof(simple_comm));
	a->com = NULL;
	a->arg = NULL;
	return a;}

void free_simple_comm(simple_comm* com) {
	if (com != NULL) {
        if (com->arg != NULL){
            free(com->arg);
        }
        if (com->com != NULL)
            free(com->com);
        free(com);
    }}

simple_comm* read_simple_comm(char* str) {
	//printf("read_simple_comm: _%s_\n", str);
	simple_comm* a = init_simple_comm();
	int i = 0;
	while(str[i] == ' ')
		i++;
	int size = 1;
	char* word = malloc(size);
	char t = str[i];
	if (strlen(str) == 0) {
		printf("  error: read_simple_comm\n  empty string");
		free_simple_comm(a);
		free(str);
		free(word);
		return NULL;
	}
	while (t != ' ' && t != '\0') {
	    word[size - 1] = t;
	    size++;
	    word = realloc(word, size);
	    i++;
	    t = str[i];
	}
	word[size - 1] = '\0';
	a->com = word;
	if (t == '\0') {
		free(str);
        a->arg = NULL;
	    return a;
	}
	i++;
    size = 1;
    word = malloc(size);
    t = str[i];
    while (t != '\0') {
        word[size - 1] = t;
        size++;
        word = realloc(word, size);
        i++;
        t = str[i];
    }
    word[size - 1] = '\0';
	a->arg = word;
	free(str);
	return a;}

conv* init_conv() {
	conv* a = malloc(sizeof(conv));
	a->next = NULL;
	a->com = NULL;
	return a;}

void free_conv(conv* a) {
	conv* temp = a;
	while (temp != NULL) {
        a = temp;
		temp = temp->next;
        if (a->com != NULL)
		  free_simple_comm(a->com);
		free(a);
	}}

conv* read_conv(char* str) {
	//printf("read_conv: _%s_\n", str);
	conv* a = init_conv();
	conv* temp = a;
	int len = strlen(str);
    for (int i = 0; i <= len; i++) {
        int size = 1;
        char* word = malloc(size);
        char t = str[i];
        while (t != '|' && t != '\0') {
            word[size - 1] = t;
            size++;
            word = realloc(word, size);
            i++;
            t = str[i];
        }
        if (size == 1) {
            printf("  error: read_conv\n  %s\n  empty com at %d position\n", str, i);
            free_conv(a);
            free(str);
            free(word);
            return NULL;
        }
        /*
        abcd_|_aa
        0123456789
        word = "abcd "
        size = 6
        
        abcd\0
        012345
        word = "abcd"
        size = 5
        */
        if (t == '\0') {
        	word[size - 1] = '\0';
        	temp->com = read_simple_comm(word);
        	if(temp->com == NULL) {
                printf("error: read_conv\n");
                free_conv(a);
                free(str);
                return NULL;
            }
            break;
        }
        else {
        	word = realloc(word, size - 1);
        	word[size - 2] = '\0';
        	temp->com = read_simple_comm(word);
        	if(temp->com == NULL) {
                printf("error: read_conv\n");
                free_conv(a);
                free(str);
                return NULL;
            }
        }
        i++;
        temp->next = init_conv();
        temp = temp->next;
    }
    free(str);
	return a;}

comm* init_comm() {
	comm* a = malloc(sizeof(comm));
	a->input = NULL;
	a->output = NULL;
	a->append_output = 0;
	a->convey = NULL;
	return a;}

void free_comm(comm* a) {
    if (a != NULL) {
        if (a->input != NULL)
	      free(a->input);
        if (a->output != NULL)
	      free(a->output);
        if (a->convey != NULL)
	      free_conv(a->convey);
	   free(a);}
	   }

comm* read_comm(char* str) {
	//printf("read_comm: _%s_\n", str);
    comm* a = init_comm();
    int in = 0;
    int out = 0;
    char* word = NULL;
    int i = 0;
    int len = strlen(str);
    if (len == 0) {
	    printf("  error: read_comm\n  empty string\n");
	    free_comm(a);
	    free(str);
	    return NULL;
	}
    if ((word = strchr(str, '<')) != NULL) {
        in = 1;
        if ((word = strchr(word + 1, '<')) != NULL) {
            printf("  error: read_comm\n  %s\n  too much input files\n", str);
            free_comm(a);
            free(str);
            return NULL;
        }
    }
    if ((word = strstr(str, ">>")) != NULL) {
        out = 2;
        if ((word = strstr(word + 2, ">>")) != NULL) {
            printf("  error: read_comm\n  %s\n  too much output files\n", str);
            free_comm(a);
            free(str);
            return NULL;
        }
    }
    else if ((word = strchr(str, '>')) != NULL) {
        out = 1;
        if ((word = strchr(word + 1, '>')) != NULL) {
            printf("  error: read_comm\n  %s\n  too much output files\n", str);
            free_comm(a);
            free(str);
            return NULL;
        }
    }
    
    if (out != 0 && str[0] == '>') {
        i = 2;
        if (str[1] == '>') {
            a->append_output = 1;
            i = 3;
        }
        if (len < i) {
	        printf("  error: read_shell_comm\n  %s\n  empty output at %d position\n", str, i + 1);
	        free_comm(a);
	        free(str);
    	    return NULL;
	    }
        int size = 1;
        char* word = malloc(size);
        while (str[i] != ' ' && str[i] != '\0' && str[i] != '<') {
            word[size - 1] = str[i];
            i++;
            size++;
            word = realloc(word, size);
        }
        word[size - 1] = '\0';
        if (str[i] == '\0') {
            printf("  error: read_comm\n  %s\n  empty conveyor\n", str);
            free_comm(a);
            free(str);
            free(word);
            return NULL;
        }
        if (str[i] == '<') {
            printf("  error: read_comm\n  %s\n  empty output at %d position\n", str, i + 1);
            free_comm(a);
            free(str);
            free(word);
            return NULL;
        }
        if (strlen(word) == 1) {
            printf("  error: read_comm\n  %s\n  empty output\n", str);
            free_comm(a);
            free(str);
            free(word);
            return NULL;
        }
        //printf("output: %s\n", word);
        a->output = word;
        i++;
        out = 10;
        if (in) {
            if (str[i] == '<') {
                i += 2;
                int size = 1;
                char* word = malloc(size);
                while (str[i] != ' ' && str[i] != '\0') {
                    word[size - 1] = str[i];
                    i++;
                    size++;
                    word = realloc(word, size);
                }
                word[size - 1] = '\0';
                if (str[i] == '\0') {
                    printf("  error: read_comm\n  %s\n  empty conveyor\n", str);
                    free_comm(a);
                    free(str);
                    free(word);
                    return NULL;
                }
                if (strlen(word) == 1) {
                    printf("  error: read_comm\n  %s\n  empty input at %d position\n", str, i + 1);
                    free_comm(a);
                    free(str);
                    free(word);
                	return NULL;
                }
            	a->input = word;
            	//printf("input: %s\n", word);
            	in = 10;
            	i++;
            }
            else {
                printf("  error: read_comm\n  %s\n  missing input at %d position\n", str, i + 1);
                free_comm(a);
                free(str);
                free(word);
                return NULL;
            }
            
        }
    }
    else if (in != 0 && str[0] == '<') {
        i = 2;
        if (len < i) {
	        printf("  error: read_shell_comm\n  %s\n  empty input at %d position\n", str, i + 1);
	        free_comm(a);
	        free(str);
    	    return NULL;
	    }
        int size = 1;
        char* word = malloc(size);
        while (str[i] != ' ' && str[i] != '\0' && str[i] != '>') {
            word[size - 1] = str[i];
            i++;
            size++;
            word = realloc(word, size);
        }
        word[size - 1] = '\0';
        if (str[i] == '\0') {
            printf("  error: read_comm\n  %s\n  empty conveyor\n", str);
            free_comm(a);
            free(str);
            free(word);
            return NULL;
        }
        if (str[i] == '>') {
            printf("  error: read_comm\n  %s\n  empty input at %d position\n", str, i + 1);
            free_comm(a);
            free(str);
            free(word);
            return NULL;
        }
        if (strlen(word) == 1) {
            printf("  error: read_comm\n  %s\n  empty input\n", str);
            free_comm(a);
            free(str);
            free(word);
            return NULL;
        }
        a->input = word;
        //printf("input: %s\n", word);
        i++;
        in = 10;
        if (out) {
            if (str[i] == '>') {
                i += 2;
                if (out == 2)
                    i++;
                int size = 1;
                char* word = malloc(size);
                while (str[i] != ' ' && str[i] != '\0') {
                    word[size - 1] = str[i];
                    i++;
                    size++;
                    word = realloc(word, size);
                }
                word[size - 1] = '\0';
                if (str[i] == '\0') {
                    printf("  error: read_comm\n  %s\n  empty conveyor\n", str);
                    free_comm(a);
                    free(str);
                    free(word);
                    return NULL;
                }
                if (strlen(word) == 1) {
                    printf("  error: read_comm\n  %s\n  empty output at %d position\n", str, i + 1);
                    free_comm(a);
                    free(str);
                    free(word);
                return NULL;
                }
            	a->output = word;
            	//printf("output: %s\n", word);
            	out = 10;
            	i++;
            }
            else {
                printf("  error: read_comm\n  %s\n  missing output at %d position\n", str, i + 1);
                free_comm(a);
                free(str);
                free(word);
                return NULL;
            }
        }
    }
    int size = 1;
    char* convey = malloc(size);
    char t = str[i];
    while (t != '>' && t != '<' && t != '\0') {
        convey[size - 1] = t;
        size++;
        convey = realloc(convey, size);
        i++;
        t = str[i];
    }
    if (size == 1) {
        printf("  error: read_comm\n  %s\n  empty conveyor at %d position\n", str, i + 1);
        free_comm(a);
        free(str);
        free(convey);
        return NULL;
    }
  	/*
  	abv -n\0
  	01234567
  	convey = "abv -n"
  	size = 6
  	
  	asdfn -n >
  	0123456789
  	convey = "asdfn -n "
  	size = 10
  	*/
    if (t == '\0') {
    	convey[size - 1] = '\0';
    }
    else {
    	convey = realloc(convey , size - 1);
    	convey[size - 2] = '\0';
    }
    //printf("%s\n", convey);
    a->convey = read_conv(convey);
    if (a->convey == NULL) {
    	printf("error: read_comm\n");
    	free_comm(a);
        free(str);
        return NULL;
    }
    if (t == '\0') {
    	free(str);
    	return a;
    }
    else {
        if (t == '>') {
            if (out == 2 && str[i + 1] == '>') {
                i += 3;
                a->append_output = 1;
            }
            else {
                i += 2;
            }
            if (i >= len) {
                printf("  error: read_comm\n  %s\n  empty output at %d position", str, i + 1);
                free_comm(a);
                free(str);
                return NULL;
            }
            t = str[i];
            int size = 1;
            char* word = malloc(size); 
            while (t != ' ' && t != '\0') {
                word[size - 1] = t;
                size++;
                word = realloc(word, size);
                i++;
                t = str[i];
            }
            /*
            abcd_
            01234
            word = "abcd"
            size = 5
            
            abcd\0
            01234
            word = "abcd"
            size = 5
            */
            word[size - 1] = '\0';
            a->output = word;
            //printf("%s\n", word);
            if (in == 1) {
                i++;
                if (i >= len) {
                    printf("  error: read_comm\n  %s\n  empty input at %d position", str, i + 1);
                    free_comm(a);
                    free(str);
                    return NULL;
                }
                t = str[i];
                if (t == '<') {
                    i += 2;
                    if (i >= len) {
                        printf("  error: read_comm\n  %s\n  empty input at %d position", str, i + 1);
                        free_comm(a);
                        free(str);
                        return NULL;
                    }
                    t = str[i];
                    int size = 1;
                    char* word = malloc(size); 
                    while (t != ' ' && t != '\0') {
                        word[size - 1] = t;
                        size++;
                        word = realloc(word, size);
                        i++;
                        t = str[i];
                    }
                    word[size - 1] = '\0';
                    a->input = word;
                    //printf("%s\n", word);
                }
                else {
                    printf("  error: read_comm\n  %s\n  missing input at %d position", str, i + 1);
                    free_comm(a);
                    free(str);
                    return NULL;
                }
            }
        }
        else if (t == '<') {
            i += 2;
            if (i >= len) {
                printf("  error: read_comm\n  %s\n  empty input at %d position", str, i + 1);
                free_comm(a);
                free(str);
                return NULL;
            }
            t = str[i];
            int size = 1;
            char* word = malloc(size); 
            while (t != ' ' && t != '\0') {
                word[size - 1] = t;
                size++;
                word = realloc(word, size);
                i++;
                t = str[i];
            }
            
            word[size - 1] = '\0';
            a->input = word;
            //printf("%s\n", word);
            if (out == 1 || out == 2) {
                i++;
                if (i >= len) {
                    printf("  error: read_comm\n  %s\n  empty output at %d position", str, i + 1);
                    free_comm(a);
                    free(str);
                    return NULL;
                }
                t = str[i];
                if (t == '>') {
                    if (out == 2 && str[i + 1] == '>') {
                        i++;
                        a->append_output = 1;
                    }
                    i += 2;
                    if (i >= len) {
                        printf("  error: read_comm\n  %s\n  empty output at %d position", str, i + 1);
                        free_comm(a);
                        free(str);
                        return NULL;
                    }
                    t = str[i];
                    int size = 1;
                    char* word = malloc(size); 
                    while (t != ' ' && t != '\0') {
                        word[size - 1] = t;
                        size++;
                        word = realloc(word, size);
                        i++;
                        t = str[i];
                    }
                    word[size - 1] = '\0';
                    a->output = word;
                    //printf("%s\n", word);
                }
                else {
                    printf("  error: read_comm\n  %s\n  missing output at %d position", str, i + 1);
                    free_comm(a);
                    free(str);
                    return NULL;
                }
            }
        }
    }
    free(str);
	return a;}

cond_comm* init_cond_comm() {
	cond_comm* a = malloc(sizeof(cond_comm));
	a->cond = '0';
	a->com = NULL;
	a->next = NULL;
	return a;}

void free_cond_comm(cond_comm* a) {
    cond_comm* temp = a;
    while (temp != NULL) {
        a = temp;
    	temp = temp->next;
        if (a->com != NULL)
    	   free_comm(a->com);
    	free(a);
    }}

cond_comm* read_cond_comm(char* str) {
	//printf("read_cond_comm: _%s_\n", str);
    cond_comm* a = init_cond_comm();
    cond_comm* temp = a;
    int len = strlen(str);
    if (len == 0) {
	    printf("  error: read_cond_comm\n  empty string\n");
	    free_cond_comm(a);
	    free(str);
	    return NULL;
	}
    for (int i = 0; i <= len; i++) {
        int begin = i;
        int end = i;
        char t = str[i];
        if (t == '&' || t == '|') {
            printf("  error: read_cond_comm\n  %s\n  bad '%c' at %d position\n", str, t, i + 1);
            free_cond_comm(a);
            free(str);
            return NULL;
        }
        if (i >= len) {
            printf("  error: read_cond_comm\n  %s\n  empty comm at %d position\n", str, i + 1);
            free_cond_comm(a);
            free(str);
            return NULL;
        }
        while ( !((t == '&' && str[i + 1] == '&') || (t == '|' && str[i + 1] == '|')) && t != '\0') {
            i++;
            t = str[i];
        }
        end = i;
/*
        abcs\0
        01234
        begin = 0
        end = 4

        abcs_&&_abc
        01234567
        begin = 0
        end = 5
*/
        if (t != '\0') {
            if (str[i + 1] == t) {
                int n = end - begin - 1;
                char* com = malloc(n + 1);
                strncpy(com, str + begin, n);
                com[n] = '\0';
                temp->com = read_comm(com);
                if (temp->com == NULL) {
                	printf("error: read_cond_com\n");
                	free_cond_comm(a);
                	free(str);
                	return NULL;
                }
                if (str[i + 2] != '\0') {
                    temp->next = init_cond_comm();
                    temp->next->cond = t;
                    temp = temp->next;
                    i += 2;
                }
                else {
                    printf("  error: read_cond_com\n  %s\n  empty comm\n", str);
                    free_cond_comm(a);
                    free(str);
                    return NULL;
                }
            }
            else {
                printf("  error: read_cond_comm\n  %s\n  bad '%c' at %d position\n", str, str[i + 1], i + 2);
                free_cond_comm(a);
                free(str);
                return NULL;
            }
        }
        else {
            int n = end - begin;
            char* com = malloc(n + 1);
            strncpy(com, str + begin, n);
            com[n] = '\0';
            temp->com = read_comm(com);
            if (temp->com == NULL) {
                printf("error: read_cond_com\n");
                free_cond_comm(a);
                free(str);
                return NULL;
            }
            break;
        }
    }
    free(str);
	return a;}

shell_comm* init_shell_comm() {
	shell_comm* a = malloc(sizeof(shell_comm));
	a->cond_com = NULL;
	a->shell_com = NULL;
	a->next = NULL;
	a->bg = 0;
	a->br = 0;
	a->cond = '0';
	return a;}

void free_shell_comm(shell_comm* a) {
    shell_comm* t = a;
    while (t != NULL) {
        a = t;
        if (a->cond_com != NULL)
	        free_cond_comm(a->cond_com);
	    if (a->shell_com != NULL)
	        free_shell_comm(a->shell_com);
	    t = t->next;
	    free(a);}}

shell_comm* read_shell_comm(char* str) {
	//printf("read_shell_comm: _%s_\n", str);
	shell_comm* a = init_shell_comm();
	shell_comm* temp = a;
	int len = strlen(str);
	if (len == 0) {
	    printf("  error: read_shell_comm\n  empty string\n");
	    free_shell_comm(a);
	    free(str);
	    return NULL;
	}
    for (int i = 0; i <= len; i++) {
        int begin = i;
        int end = i;
        if (i >= len) {
            printf("  error: read_shell_comm\n  empty comm\n");
            free_shell_comm(a);
            free(str);
            return NULL;
        }
        char t = str[i];
        if (t == ';' || t == '&') {
            printf("  error: read_shell_comm\n  %s\n  bad '%c' at %d position\n", str, t, i);
            free_shell_comm(a);
            free(str);
            return NULL;
        }
        if (t == '(') {
            temp->br = 1;
            i += 2;
            t = str[i];
            begin = i;
            int balance = 1;
            while (balance != 0) {
                if (t == '(')
                    balance++;
                else if (t == ')')
                    balance--;
                i++;
                t = str[i];
            }
            /*
            ( dsa )_
            01234567
            begin = 2
            end = 7
            n = 3 = end - begin - 2
            
            ( )\0
            0123
            */
            end = i;
            int n = end - begin - 2;
            if (n <= 0) {
            	printf("  error: read_shell_comm\n  %s\n  empty comm\n", str);
            	free_shell_comm(a);
            	free(str);
            	return NULL;
            }
            char* sh_com = malloc(n + 1);
            strncpy(sh_com, str + begin, n);
            sh_com[n] = '\0';
            printf("com:_%s_\n", sh_com);
            if (len != i && str[i + 1] != '\0' && str[i + 2] != '\0') 
                temp->next = init_shell_comm();
            if (t != '\0') {
                if (str[i + 1] == '&' && str[i + 2] != '&') {
                    temp->bg = 1;
                    i += 2;
                }
                else if (str[i + 1] == '&' && str[i + 2] == '&') {
                    temp->next->cond = '&';
                    i += 3;
                }
                else if (str[i + 1] == '|' && str[i + 2] == '|') {
                    temp->next->cond = '|';
                    i += 3;
                }
                else {
                    i += 1;
                }
            }
            temp->shell_com = read_shell_comm(sh_com);
            if (temp->shell_com == NULL) {
                printf("error: read_shell_comm\n");
                free_shell_comm(a);
                free(str);
                return NULL;
            }
            if (temp->next != NULL) {
                temp = temp->next;
            }
            else 
                break;
        } 
        else if (strchr(str + i, '(') != NULL) {
            while (t != '\0' && t != ';' && t != '(') {
                if (t == '&' && str[i + 1] != '&' && (i > 0 && str[i - 1] != '&'))
                    break; 
                i++;
                t = str[i]; 
            }
            /*
            adff_&&_(
            012345678
            begin = 0
            end = 8
            n = 4
            
            adsb_&_     // _;_
            0123456
            begin = 0
            end = 5
            n = 4

            adff_&_(
            01234567
            begin = 0
            end = 5
            n = 4

            asdf\0
            01234
            begin = 0
            end = 4
            n = 4
            */
            end = i;
            if (t == '(') { // d_&&_( // d_||_(
                int n = end - begin - 4;
                char* com = malloc(n + 1);
                strncpy(com, str + begin, n);
                com[n] = '\0';
                temp->cond_com = read_cond_comm(com);
                if (temp->cond_com == NULL) {
                    printf("error: read_shell_comm\n");
                    free_shell_comm(a);
                    free(str);
                    return NULL;
                }
                temp->next = init_shell_comm();
                temp = temp->next;
                temp->cond = str[i - 2];
                i--; // back 1 space so next cycle will start sith '('
            }
            else { // d_&_( // d_;_( // d_&_a // d_;_a
                if (t == '&') 
                    temp->bg = 1;
                int n;
                if (t == '\0') 
                    n = end - begin;
                else {
                    n = end - begin - 1;
                }
                char* com = malloc(n + 1);
                strncpy(com, str + begin, n);
                com[n] = '\0';
                temp->cond_com = read_cond_comm(com);
                if (temp->cond_com == NULL) {
                    printf("error: read_shell_comm\n");
                    free_shell_comm(a);
                    free(str);
                    return NULL;
                }
                if (t != '\0' && str[i + 1] != '\0' && str[i + 2] != '\0') {
                    temp->next = init_shell_comm();
                    temp = temp->next;
                    i++; // skip 1 space
                }
            }
        } 
        else {
            while (t != '\0' && t != ';') {
                if (t == '&' && str[i + 1] != '&' && (i > 0 && str[i - 1] != '&'))
                    break; 
                i++;
                t = str[i];	
            } 
            end = i;
            if (t == '&')
                temp->bg = 1;
            int n = 0;
            if (t == '\0')
            	n = end - begin;
            else
            	n = end - begin - 1;
            char* com = malloc(n + 1);
            /*
            abcds\0
            0123456
            begin = 0
            end = 5
            
            abcdsaf_;_
            0123456789
            begin = 0
            end = 8
            */
            char* x = str + begin;
            strncpy(com, x, n);
            com[n] = '\0';
            temp->cond_com = read_cond_comm(com);
            if (temp->cond_com == NULL) {
                printf("error: read_shell_comm\n");
                free_shell_comm(a);
                free(str);
                return NULL;
            }
            if (t != '\0' && str[i + 1] != '\0' && str[i + 2] != '\0') {
                temp->next = init_shell_comm();
                temp = temp->next;
                i++; // skip 1 space
            }
            else {
            	break;
            }
            
        }
    }
	free(str);
	return a;}

int if_special(char c) {
    int flag = 0;
    if (c == '>')
        flag++;
    else if (c == '<')
        flag++;
    else if (c == '&')
        flag++;
    else if (c == '|')
        flag++;
    else if (c == ';')
        flag++;
    return flag;}

int check_str(char* str) {
    int i = 0;
    int len = strlen(str);
    while (i < len) {
        int size = 1;
        char* word = malloc(size);
        char t = str[i];
        while (i < len && t != ' ') {
            word[size - 1] = t;
            size++;
            word = realloc(word, size);
            i++;
            t = str[i];
        }
        word[size - 1] = '\0';
        if (size == 3) {
            if ((if_special(word[0]) && word[0] != word[1]) ||
                (if_special(word[1]) && word[0] != word[1]) ||
                (word[0] == word[1] && (word[0] == ';' || word[0] == '<'))) {
                printf("error: unknown symbol '%s'\n", word);
                free(word);
                return 0;
                }
            else {
            	free(word);
            }
        }
        else if (size > 3) {
            if (strchr(word, '&') != NULL ||
                strchr(word, '|') != NULL ||
                strchr(word, '>') != NULL ||
                strchr(word, '<') != NULL ||
                strchr(word, ';') != NULL) {
                printf("error: unknown symbol '%s'\n", word);
                free(word);
                return 0;
            }
            else {
            	free(word);
            }
        }
        else 
        	free(word);
        i++;
    }
    return 1;}

char* read_str() {
    int size = 1;
    char* a = malloc(size);
    char t = getchar();
    char prev = ' ';
    int left = 0;
    int right = 0;
    int flag = 0;
    if (t == '(' || t == '<' || t == ';') {
        a = realloc(a, size + 2);
        a[size - 1] = t;
        a[size] = ' ';
        size += 2;
        if (t == '(')
            left++;
        prev = ' ';
        t = getchar();
    }
    else if (t == ')') {
        free(a);
        printf("error: unbalanced brackets at symbol %d\n", size);
        while (t != '\n' && t != '\0')
        	t = getchar();
        return NULL;
    }
    while (t != '\n' && t != '\0') {
        if (t == ' ' && prev == ' ') {
            prev = t;
            t = getchar();
            continue;
        }
            
        if (t == '(') {
            if (prev != ' ' && prev != ')' && prev != '(') {
                a[size - 1] = ' ';
                size++;
                a = realloc(a, size);
                prev = ' ';
            }
            left++;
            flag = 1;
            
        }
        else if (t == ')') {
            if (prev != ' ' && prev != ')' && prev != '(') {
                a[size - 1] = ' ';
                size++;
                a = realloc(a, size);
                prev = ' ';
            }
            right++;
            flag = 1;
        }
        else if (if_special(t) && prev != ' ') {
            if (t == prev && (t == '>' || t == '&' || t == '|')) {
                flag = 1;
            }
            else {
                a[size - 1] = ' ';
                size++;
                a = realloc(a, size);
                prev = ' ';
                flag = 1;
            }
        }
        else if (if_special(prev) && t != prev && t != ' ') {
            a[size - 1] = ' ';
            size++;
            a = realloc(a, size);
            prev = ' ';
        }
        a[size - 1] = t;
        size++;
        a = realloc(a, size);
        prev = t;
        t = getchar();
        if (flag && !(t == '>' || t == '&' || t == '|')) {
            a[size - 1] = ' ';
            size++;
            a = realloc(a, size);
            prev = ' ';
        }
        flag = 0;
        /*
        if (left < right) {
            a[size - 1] = '\0';
            printf("error: less opened brackets than closed\n");
            free(a);
            while (t != '\n' || t != '\0')
                t = getchar();
            return NULL;
        }
        */
        
    }
    if (a[size - 2] != ' ') {
        a[size - 1] = '\0';
    }
    else {
        a = realloc(a, size - 1);
        a[size - 2] = '\0';
    }
    if (left != right) {
        free(a);
        printf("error: unbalanced brackets\n(' = %d\n')' = %d\n", left, right);
        return NULL;
    }
    if (strcmp(a, "exit") == 0) {
        printf("program finished\n");
        free(a);
        exit(0);
    }
    
    if (check_str(a))
        return a;
    else {
        free(a);
        return NULL;
    }}

int depth = 0;
void print_simple_comm(simple_comm* a) {
	if (a->com != NULL) {
		printf("%*s        com: %s\n", depth, "", a->com);
		if (a->arg != NULL)
			printf("%*s        arg: %s\n", depth, "", a->arg);
		else
			printf("%*s        arg:\n", depth, "");
	}
	else
		printf("%*s        com:\n", depth, "");}

int conv_cnt = 1;
void print_conv(conv* a) {
	conv* t = a;
	while (t != NULL) {
		printf("%*s      conv %d:\n", depth, "", conv_cnt++);
		if (t->com != NULL) 
			print_simple_comm(t->com);
		t = t->next;
	}
	conv_cnt = 1;}

void print_comm(comm* a) {
	printf("%*s    comm: append = %d\n", depth, "", a->append_output);
	if (a->input != NULL) 
		printf("%*s    input: \"%s\" ", depth, "", a->input);
	else
		printf("%*s    no input; ", depth, "");
	if (a->output != NULL) 
		printf("output: \"%s\"\n", a->output);
	else
		printf("no output\n");
	if (a->convey != NULL)
		print_conv(a->convey);}

void print_cond_comm(cond_comm* a) {
	cond_comm* t = a;
	while (t != NULL) {
		printf("%*s  cond_comm: cond: '%c'\n", depth, "", t->cond);
		if (t->com != NULL)
			print_comm(t->com);
		t = t->next;
	}}

int balance_print = 0;
int shell_comm_cnt = 1;
void print_shell_comm(shell_comm* a) {
	shell_comm* t = a;
	while (t != NULL) {
		printf("%*sshell_command %d:\n%*scondition: '%c', bg = %d, br = %d\n", 
                depth, "", shell_comm_cnt++, depth, "", t->cond, t->bg, t->br);
        if (t->br) {
            printf("%*s(\n", depth, "");
            balance_print++;
        }
		if (t->cond_com != NULL) {
			print_cond_comm(t->cond_com);
            if (balance_print != 0 && t->next == NULL) {
                balance_print--;
            }
		}
		else if (t->shell_com != NULL) {
            depth += 4;
			print_shell_comm(t->shell_com);
            printf("%*s)\n", depth - 4, "");
            depth -= 4;
		}
		t = t->next;
	}
	shell_comm_cnt = 1;}

void exec_simple_comm(simple_comm* a) {
    char* com = a->com;
    if (com == NULL) {
        printf("  error: exec_simple_comm\n  empty com\n");
        return;
    }
    if (a->arg == NULL) {
        //printf("%s\n", com);
        char** arg = malloc(2);
        int n = strlen(com);
        arg[0] = malloc(n + 1);
        strcpy(arg[0], com);
        arg[0][n] = '\0';
        arg[1] = NULL;
        execvp(com, arg);
        return;
    }
    char* str = a->arg;
    int n = 1;
    for (int i = 0; i < strlen(str); i++)
        if (str[i] == ' ')
            n++;
    //printf("n:%d\n", n);
    char** arg = malloc(n + 2);
    arg[0] = malloc(strlen(com) + 1);
    strcpy(arg[0], com);
    arg[0][strlen(com)] = '\0';
    int k = 0;
    /*
    0 = com
    1 = arg1
    ...
    n = arg2
    n + 1 = NULL
    */
    for (int i = 1; i < n + 1; i++) {
        int j = 0;
        arg[i] = malloc(j + 1);
        while (str[k] != ' ' && str[k] != '\0') {
            arg[i][j] = str[k];
            j++;
            k++;
            arg[i] = realloc(arg[i], j + 1);
        }
        k++;
        arg[i][j] = '\0';
        //printf("%d string:_%s_\n", i + 1, arg[i]);
    }
    arg[n + 1] = NULL;
    //printf("%d string:%d\n", n + 1, arg[n + 1] == NULL);
    execvp(com, arg);}

int count_coms(conv* a) {
    int cnt = 0;
    conv* temp = a;
    while (temp != NULL && temp->com != NULL) {
        cnt++;
        temp = temp->next;
    }
    return cnt;}

int exec_conv(conv* a, char* in, char* out, int append_output) {
    conv* temp = a;
    int n = count_coms(a);
    //printf("coms counted:%d\n", n);
    int pid[n];
    int st = 0;
    int in_fd = 0;
    int out_fd = 0;
    if (in) {
        in_fd = open(in, O_RDONLY | O_EXCL);
        if (in_fd == -1) {
            printf("error: exec_conv\n%s\ninput file error\n", in);
            return -1;
        }
    }
    if (out) {
        printf("ap:%d\n", append_output);
        if (append_output) {
            out_fd = open(out, O_WRONLY | O_CREAT | O_APPEND, 0666);
        }
        else {
            out_fd = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
        if (out_fd == -1) {
            printf("error: exec_conv\n%s\noutput file error\n", out);
            return -1;
        }
    }
    //printf("vse norm\n");
    int pid_fd[2];
    pipe(pid_fd);
    int og_pid = fork();
    if (!og_pid) {
        og_pid = getppid();
        close(pid_fd[0]);
        for (int i = 0; i < n; i++) {
            int fd[2];
            pipe(fd);
            pid[i] = fork();    
            if (pid[i]) { // father
                if (in && (i == 0)) {
                    dup2(in_fd, 0); // input
                    close(in_fd);
                }
                close(fd[0]);
                if (i == n - 1) {
                    int x = getpid();
                    write(pid_fd[1], &x, sizeof(int));
                    if (out) {
                        dup2(out_fd, 1); // output
                        close(out_fd);
                    }   
                }
                else {
                    dup2(fd[1], 1); // output
                }
                exec_simple_comm(temp->com);
                exit(-1);
                return -1;
            }
            else {        // son
                pid[i] = getppid();
                write(pid_fd[1], &pid[i], sizeof(int));
                dup2(fd[0], 0); // input
                close(fd[1]);
                if (i == n - 1) {
                    exit(0);
                }
                temp = temp->next;
            }
        }
    }
    else {
        if (in)
            close(in_fd);
        if (out)
            close(out_fd);
        close(pid_fd[1]);
        for (int i = 0; i < n; i++) {
            read(pid_fd[0], &pid[i], sizeof(int));
            //printf("read+%d\n", pid[i]);
        }
        //sleep(5);
        for (int i = 0; i < n; i++) {
            waitpid(pid[i], &st, 0);
            //printf("wait+%d\n", st);
        }
    }
    return st;}

int exec_comm(comm* a) {
    int x = exec_conv(a->convey, a->input, a->output, a->append_output);
    return x;}

int exec_cond_comm(cond_comm* a) {
    cond_comm* temp = a;
    int st = 0;
    //int i = 1;
    while (temp != NULL) {
    	//printf("%dst:%d\n", i++, st);
        char cnd = temp->cond;
        if (cnd != '&' && cnd != '|') {
            st = exec_comm(temp->com);
        }
        else if (cnd == '|' && st != 0) {
            st = exec_comm(temp->com);
        }
        else if (cnd == '&' && st == 0) {
            st = exec_comm(temp->com);
        }
        else {
            break;
        }
        temp = temp->next;
    }
    return st;}

int bg_cnt = 0;
void wait_bg() {
    for (int i = 0; i < bg_cnt; i++)
        wait(0);}

int exec_shell_comm(shell_comm* a);

typedef struct bgpid {
	struct bgpid* next;
	int pid;
} bgpid;

int exec_bg(shell_comm* a) {
	int pid = fork();
	if (pid) {
		waitpid(pid, 0, 0);
	}
	else {
		int fd0 = open("dev/null", O_RDONLY);
		dup2(fd0, 0);
		//int fd1 = open("dev/null", O_WRONLY);
		//dup2(fd, 1);
		//close(fd1);
		close(fd0);
		signal(SIGINT, SIG_IGN);
		pid = fork();
		if (pid) {
			exit(0);
		}
		else {
			if (a->br)
				exec_shell_comm(a->shell_com);
			else
				exec_cond_comm(a->cond_com);
			exit(0);
		}
	}
    return 0;}

int exec_shell_comm(shell_comm* a) {
    shell_comm* t_nx = a;
    int st = 0;
    while (t_nx != NULL) {
        shell_comm* t_sh = t_nx;
        if (t_sh->bg) {
        	exec_bg(t_sh);
        }
        else if (t_sh->shell_com != NULL) {
        	if ((t_sh->cond != '&' && t_sh->cond != '|') || 
        	    (t_sh->cond == '&' && st == 0) || 
        	    (t_sh->cond == '|' && st != 0)) {
            	st = exec_shell_comm(t_sh->shell_com);
            }
            else {
            	return -1;
            }
        }
        else if (t_sh->cond_com != NULL) {
            if ((t_sh->cond != '&' && t_sh->cond != '|') || 
        	    (t_sh->cond == '&' && st == 0) || 
        	    (t_sh->cond == '|' && st != 0)) {
            	st = exec_cond_comm(t_sh->cond_com);
            }
            else {
            	return -1;
            }
        }
        t_nx = t_nx->next;
    }
    return st;}

int main() {
// parser test
/* 
	char* str = read_str();
	printf("%s_\n\n", str);
	shell_comm* a = read_shell_comm(str);    
    if (a != NULL) {
    	print_shell_comm(a);
        free_shell_comm(a);
    }
    else {
    	printf("NULL\n");
    }
*/
// read+exec simple comm test
/*
    char* str = read_str();
    simple_comm* a = read_simple_comm(str);
    print_simple_comm(a);
    exec_simple_comm(a);
*/
// exec_simple_comm test
/*
    simple_comm* a = init_simple_comm();
    a->com = malloc(3);
    a->com[0] = 'l';
    a->com[1] = 's';
    a->com[2] = '\0';
    a->arg = NULL;
    if (fork()) {
        sleep(1);
    }
    else {
        exec_simple_comm(a);
    }
*/
// exec_conv test
/*
    char* str = read_str();
    while (str != NULL) {
    	comm* a = read_comm(str);
    	print_comm(a);
    	sleep(3);
    	int x = exec_comm(a);
    	printf("st:%d\n", x);
    	str = read_str();
    }
*/
// exec_cond_comm test
/*
	char* str = read_str();
	cond_comm* a = read_cond_comm(str);
	print_cond_comm(a);
	int st = exec_cond_comm(a);
	printf("st:%d\n", st);
	free_cond_comm(a);
*/
// exec_shell_comm test

	
	while (1) {
		char* str = read_str();
		while (str == NULL) {
			//printf("bad\n");
			str = read_str();
		}
		if (strcmp(str, "exit") == 0) {
			printf("program finished\n");
			free(str);
			exit(0);
		}
		//printf("executing:%s\n", str);
		shell_comm* a = read_shell_comm(str);
		//print_shell_comm(a);
		exec_shell_comm(a);
		//int st = exec_shell_comm(a);
		//printf("st:%d\n", st);
		free_shell_comm(a);
	}
	
    return 0;}






