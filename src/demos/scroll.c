#include "stdlib.h"
#include "cli_prompt.h"
#include "portable_utils.h"


int main(void) {

    printf("Hello world.\n");

    CliPrompt cli_prompt = { 0 };
    CliPrompt_setup(&cli_prompt);

    time_t last_print = 0;

    while (1) {
        time_t now = time(NULL);
        if (now >= last_print) {
            last_print = now + 5;

            CliPrompt_print_line(&cli_prompt, "[status] tick %ld\n", now);
        }

        int i = 5;
        while (CliPrompt_handle_prompt(&cli_prompt)) {
            /*printf("\nCommand: [%s]\n", cli_prompt.input.cstr);*/
            CliPrompt_print_line(&cli_prompt, "Command: [%s]\n", cli_prompt.input.cstr);
            CliPrompt_clear(&cli_prompt);
            if (--i, i < 0) { break; }
        }

        sleep_ms(50);
    }
    return 0;
}
