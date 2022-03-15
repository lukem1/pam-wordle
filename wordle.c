
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
                if (word_count == n && word != NULL) {
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

// Check if word is in dict
// Returns 1 if yes, 0 if no, negative int on error
int check_word(char *dict, char *word) {
    FILE *stream;
    if(stream = fopen(dict, "r")) {
        // Read words from dict
        char w[16];
        while (fgets(w, 16, stream) != NULL) {
            // Remove newlines
            int wlen = strlen(w);
            if (w[wlen-1] == '\n') {
                w[wlen-1] = '\0';
            }

            if (strlen(w) == strlen(word) && strncmp(w, word, strlen(word)) == 0) {
                return 1; // Word is in dict
            }
        }
        fclose(stream);
    } else {
        return -1; // Failed to read dict
    }

    return 0; // Word is not in dict
}


// Handles a round of guessing
// Prompts for a guess, checks if the guess is valid, and checks if the guess is correct
// Displays the result of the guess as follows: 
// ? - incorrect char, * - correct char in wrong location, or the char itself if it is correct and properly placed
// Returns 1 on a valid and correct guess, 0 on a valid and incorrect guess, or a negative int on error
int wordle_guess(pam_handle_t *pamh, char* word) {
    int valid = 0;
    char *resp = NULL;

    while (valid == 0) {
        int retval;
        retval = pam_prompt(pamh, PAM_PROMPT_ECHO_ON, &resp, "Word: ");
        if (retval == PAM_SUCCESS) {
            if (strlen(word) == strlen(resp)) {
                int known = check_word(DICT, resp);
                if (known == 1) {
                    valid = 1;
                } else if (known == 0) {
                    pam_info(pamh, "Invalid guess: unkown word.");
                } else {
                    pam_info(pamh, "Warning: error reading dictionary.");
                    valid = 1;
                }
            } else {
                pam_info(pamh, "Invalid guess: guess length != word length.");
            }
        } else {
            return -1;
        }
    }
    
    // Build the next hint based on the guess
    char hint[strlen(word)];

    // Handle perfect and completely incorrect guesses
    for (size_t i = 0; i < strlen(word); i++) {
        if (word[i] == resp[i]) { // Letter is correct and properly placed
            hint[i] = word[i];
        } else {
            int misplacements = 0;
            for (size_t j = 0; j < strlen(word); j++) {
                if (resp[i] == word[j] && word[j] != resp[j]) {
                    misplacements += 1;
                }
            }
            
            int handled_misplacements = 0;
             for (size_t j = 0; j < i; j++) {
                 if (resp[i] == resp[j] && word[j] != resp[j]) {
                     handled_misplacements += 1;
                 }
             }
            if (handled_misplacements < misplacements) {
                hint[i] = '*'; // Letter is correct but in the wrong place
            } else {
                hint[i] = '?'; // Letter is incorrect
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
    int rounds = 6; // Maximum number of guessing rounds

    pam_info(pamh, "--- Welcome to PAM-Wordle! ---\n\nA five character [a-z] word has been selected.\nYou have %d attempts to guess the word.\n\nAfter each guess you will recieve a hint which indicates:\n? - what letters are wrong.\n* - what letters are in the wrong spot.\n[a-z] - what letters are correct.\n", rounds);

    char word[5] = "linux";
    int word_count = fetch_word(DICT, 0, NULL);

    if (word_count > 0) {
        srand(time(0));
        int n = rand() % word_count;

        fetch_word(DICT, n, word);
    } else {
        pam_info(pamh, "Warning: error reading dictionary.");
    }

    for (int i = 0; i < rounds; i++) {
        pam_info(pamh, "--- Attempt %d of %d ---", i + 1, rounds);

        int status = wordle_guess(pamh, word);

        if (status == 1) {
            return PAM_SUCCESS;
        } else if (status < 0) {
            return PAM_AUTH_ERR;
        }
    }

    pam_info(pamh, "You lose.\nThe word was: %s", word);

    return PAM_AUTH_ERR;
}

int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_IGNORE;
}

int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
