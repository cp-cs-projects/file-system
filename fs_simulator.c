#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

//global vars
	int idx = 0;
    int nidx = 0;
    int i_count = 0;
    int name_count = 0;
    int flag = 0;
    char c = 0;
    char *fs;
    FILE *fcontents;
    FILE *new;
    FILE *new_entry;
    char dstr[36];
    char *fname;
    FILE *inodes_lst;

struct inode {
	uint32_t index;
    char type;
};

struct dfile {
	uint32_t inode_num;
	char dname[32];
};

struct input {
	char cmd[6];
	char name[32];
};


char *uint32_to_str(uint32_t i)
{
   int length = snprintf(NULL, 0, "%lu", (unsigned long)i);       // pretend to print to a string to determine length
   char* str = malloc(length + 1);                        // allocate space for the actual string
   snprintf(str, length + 1, "%lu", (unsigned long)i);            // print to string
   return str;
}
//struct vars
    struct inode inodes[1024];
    struct dfile dir;
    struct inode cdir;
    struct input i = {0,0};

void get_command(char *i) {
	char c;
	int idx = 0;
	printf("> ");
    while (idx < 6 && ((c = fgetc(stdin)) != ' ') && (c != '\n')) {  //get command
        *i = c;
		*i++;
        idx++;
    }
}

void update_lsts(FILE *inodes_lst, struct inode *inodes, int i_count, char c) {
	inodes_lst = fopen("inodes_list","ab");
	struct inode new_i = {i_count, c};
	fwrite(&new_i, 5, 1, inodes_lst);
	*inodes = new_i;
	fclose(inodes_lst);
}

void update_dir(FILE *new_entry, char* fname, int i_count, char *name) {
		struct dfile new = {0,0};
		int idx = 0;
		new.inode_num = i_count;
		
		for (idx = 0; idx < 32; idx++) {
			new.dname[idx] = *name;
			*name++;
		}
		if (*name != '\0') {
			new.dname[32] = '\0';
		}
		
		new_entry = fopen(fname, "ab");
		fwrite(&new, sizeof(struct dfile), 1, new_entry);
        fclose(new_entry);
}


int main(int argc, char* argv[])
{
// REQ 1: take command arg (str) of directory name, verify existence*/
	
	if (argc != 2) {
		printf("ERROR: No directory name provided\n");
    		return 0;
	}

	int ch = chdir(argv[1]);
	if (ch == -1) {
		printf ("ERROR: Unable to open directory: %s\n", argv[1]);
        	return 0;
	}
 	
// REQ 2: load inodes from inodes_list into internal list, check bounds and indicators
	inodes_lst = fopen("inodes_list","rb");

	while (fread(&inodes[idx], 5, 1, inodes_lst) != 0) {
		if ((inodes[idx].index > 1023) || ((inodes[idx].type != 'f') && (inodes[idx].type != 'd'))) {
			printf("Invalid inode."); //idx and count not incremented for invalids
		}
		else {
			idx++;	
			i_count++;
		}
    }	
	fclose(inodes_lst);

/* REQ 3: start at inode 0, if not a directory then report error and terminate. 
track curr_directory (0) */
 
	cdir = inodes[0];
    if (cdir.type != 'd') {
        printf("ERROR: not a directory.");
        return 0;
    }
	
// REQ 4: commands
	get_command(i.cmd); // pointer to i.cmd[0]

	while (strcmp(i.cmd,"exit") != 0) {
		//ls
		if (strcmp(i.cmd,"ls") == 0) {
			fname = uint32_to_str(cdir.index); //opening current directory
            fcontents = fopen(fname,"rb");
			free(fname);	
		while (fread(&dir, 36, 1, fcontents) != 0) {
				printf("%d %s\n", dir.inode_num, dir.dname);
                }
			fclose(fcontents);
		}

		else if (strcmp(i.cmd, "cd") == 0 || strcmp(i.cmd, "mkdir") == 0 || strcmp(i.cmd, "touch") == 0) {
		//if not ls, get command <name>
			idx = 0;
			memset(i.name,0, 32);
			name_count = 0;
    		while (idx < 32 && ((c = fgetc(stdin)) != '\n')) {
        		i.name[idx] = c;
        		idx++;
				name_count++;
    		}
			if (name_count == 32) {
				i.name[32] = '\0';
			}
		
		//cd
			if (strcmp(i.cmd,"cd") == 0) {
				fname = uint32_to_str(cdir.index); /* setting idx of current directory */
				fcontents = fopen(fname,"rb");
				free(fname);
				struct inode prevdir = cdir;
				while (fread(&dir, 36, 1, fcontents) != 0) {
					if (strcmp(dir.dname, i.name) == 0) {
						if (inodes[dir.inode_num].type == 'd') {
							cdir = inodes[dir.inode_num];
							 break;
						}
					}
				}
				if(prevdir.index == cdir.index) {
					printf("ERROR: directory info incorrect \n");
				}
				fclose(fcontents);
			}
		
		//mkdir
			struct dfile curr;
			struct dfile parent;
			if (strcmp(i.cmd, "mkdir") == 0) {
				if (i_count < 1024) {
					//initializing curr and parent
					curr.inode_num = i_count;
					parent.inode_num = cdir.index;
					curr.dname[0] = '.';
					curr.dname[1] = 0;
					parent.dname[0] = '.';
					parent.dname[1] = '.';
					for (idx = 2; idx < 32; idx++) {
						curr.dname[idx] = 0;
						parent.dname[idx] = 0;
					}

					//1. creating new file representation
					fname = uint32_to_str(i_count);
					new = fopen(fname, "wb");
					free(fname);
					fwrite(&curr, sizeof(struct dfile), 1, new);
					fwrite(&parent, sizeof(struct dfile), 1, new);
					fclose(new);

					//2. writing to curr directory
					fname = uint32_to_str(cdir.index);
					update_dir(new_entry, fname, i_count, i.name);
					free(fname);

					//3. add to actual inodes_lst, update internal lst
					update_lsts(inodes_lst, &inodes[i_count], i_count, 'd');
					i_count++;
				}
			}
		//touch
			if (strcmp(i.cmd, "touch") == 0) {
				fname = uint32_to_str(cdir.index); /* setting idx of current directory */
                fcontents = fopen(fname,"rb");
				free(fname);
                while (fread(&dir, 36, 1, fcontents) != 0) {
                    flag = 0;
					if (strcmp(dir.dname, i.name) == 0) {
                        flag = 1;
						break;
                    }
                }
                fclose(fcontents);

				if (i_count < 1024 && flag == 0) {
					//1. create new file rep: ascii file name \n
					fname = uint32_to_str(i_count);
                    new = fopen(fname, "wb");
					free(fname);
					char c = '\n';
					fwrite(&i.name,name_count,1,new); 
					fwrite(&c,1,1,new);
					fclose(new);
					
					//2. write to curr directory
					fname = uint32_to_str(cdir.index);
                    update_dir(new_entry, fname, i_count, i.name);
					free(fname);
					//3. add to actual inodes lst and update internal list 
					update_lsts(inodes_lst, &inodes[i_count], i_count, 'f');
                    i_count++;
				}
			}
		}
		if (strcmp(i.cmd, "touch") != 0 && strcmp(i.cmd, "mkdir") != 0 && strcmp(i.cmd, "cd") != 0
			&& strcmp(i.cmd, "ls") != 0 ) {
			printf("invalid command\n");
			printf("%s", i.cmd);	
		}
		//reset and receive new command
	    memset(i.cmd,0, 12);
		get_command(i.cmd);
	}
	//exit
	return 0;
}	
