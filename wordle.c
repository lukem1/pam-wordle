
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

#include <security/pam_modules.h>
#include <security/pam_ext.h>

char DICT[] = "/usr/share/dict/words";

// Fetches a 5 character word from a dict
// Places the nth word in the char[] pointed to by word
// Returns the count of 5 character words in the dict on success or a negative int on failure
int fetch_word(char* dict, int n, char* word) {
    int word_count = 0;

    int err;
    regex_t regex;
    err = regcomp(&regex, "^[abcdefghijklmnopqrstuvwxyz]{5}$", REG_EXTENDED);
    if (err != 0) {
        return -1; // Failed to compile regex
    }

    // Read dict
    FILE *stream;
    if(stream = fopen(dict, "r")) {
        // Read words from dict
        char w[16];
        int rmatch;
        while (fgets(w, 16, stream) != NULL) {
            // Remove newlines
            int wlen = strlen(w);
            if (w[wlen-1] == '\n') {
                w[wlen-1] = '\0';
            }

            // If word matches regex add it to the count
            rmatch = regexec(&regex, w, 0, NULL, 0);
            if (rmatch == 0) {
                if (word_count == n) {
                    word = w;
                }
                word_count += 1;
            }
        }
        fclose(stream);
    } else {
        return -2; // Failed to read dict
    }

    return word_count;
}

int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    
    int retval;
    char *resp = NULL;
    retval = pam_prompt(pamh, PAM_PROMPT_ECHO_ON, &resp, "Continue? (y/n)\n:");

    if (retval == PAM_SUCCESS && strncmp("y", resp, 1) == 0) {
        char word[5];
        int word_count = fetch_word(DICT, 0, word);
        pam_info(pamh, "word count: %d", word_count);
        return PAM_SUCCESS;
    }

    return PAM_AUTH_ERR;
}

int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_IGNORE;
}

int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
