
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <time.h>

#include <security/pam_modules.h>
#include <security/pam_ext.h>

char DICT[] = "/usr/share/dict/words";

// Fetches a 5 character word from a dict
// Places the nth word in the char[] pointed to by word (assumes word is not null)
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
                    for (size_t i = 0; i < strlen(word) && i < strlen(w); i++) {
                        word[i] = w[i];
                    }
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


// Handles a round of guessing
// Prompts for a guess, checks if the guess is valid, and checks if the guess is correct
// Displays the result of the guess as follows: 
// X - incorrect char, * - correct char in wrong location, or the char itself if it is correct and properly placed
// Returns 1 on a valid and correct guess, 0 on a valid and incorrect guess, or a negative int on error
int wordle_guess(pam_handle_t *pamh, char* word, int round) {
    pam_info(pamh, "--- Attempt %d of 6 ---", round + 1);

    int valid = 0;
    char *resp = NULL;

    while (valid == 0) {
        int retval;
        retval = pam_prompt(pamh, PAM_PROMPT_ECHO_ON, &resp, "Word: ");
        if (retval == PAM_SUCCESS) {
            // TODO: Should also check if the guess is a known word
            if (strlen(word) == strlen(resp)) {
                valid = 1;
            } else {
                pam_info(pamh, "Invalid guess: guess length != word length.");
            }
        } else {
            return -1;
        }
    }
    
    // Build the next hint based on the guess
    char hint[strlen(word)];
    for (size_t i = 0; i < strlen(word); i++) {
        if (word[i] == resp[i]) { // Letter is correct and properly placed
            hint[i] = word[i];
        } else {
            int word_occurances = 0;
            for (size_t j = 0; j < strlen(word); j++) {
                if (resp[i] == word[j]) {
                    word_occurances += 1;
                }
            }

            int hint_occurances = 0;
            for (size_t j = 0; j < strlen(hint); j++) {
                if (resp[i] == hint[j]) {
                    hint_occurances += 1;
                }
            }

            if (hint_occurances < word_occurances) {
                hint[i] = '*'; // Letter is correct but in the wrong place
            } else {
                hint[i] = 'X'; // Letter is incorrect
            }
        }
    }

    if (strncmp(hint, word, strlen(word)) == 0) { // Guess is correct
        pam_info(pamh, "Correct!");
        return 1;
    }

    pam_info(pamh, "Hint->%s", hint);
}

int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    
    int retval;
    char *resp = NULL;
    retval = pam_prompt(pamh, PAM_PROMPT_ECHO_ON, &resp, "Continue? (y/n)\n:");

    if (retval == PAM_SUCCESS && strncmp("y", resp, 1) == 0) {
        char word[5] = "linux";
        int word_count = fetch_word(DICT, 0, word);
        pam_info(pamh, "word count: %d", word_count);

        srand(time(0));
        int wi = rand() % word_count;

        fetch_word(DICT, wi, word);
        pam_info(pamh, "word: %s", word);

        for (int i = 0; i < 6; i++) {
            int status = wordle_guess(pamh, word, i);

            if (status == 1) {
                return PAM_SUCCESS;
            } else if (status < 0) {
                return PAM_AUTH_ERR;
            }
        }
    }

    return PAM_AUTH_ERR;
}

int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_IGNORE;
}

int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
