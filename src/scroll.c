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

        CliPrompt_handle_prompt(&cli_prompt);
        if (cli_prompt.event_submit) {
            /*printf("\nCommand: [%s]\n", cli_prompt.input.cstr);*/
            CliPrompt_print_line(&cli_prompt, "Command: [%s]\n", cli_prompt.input.cstr);
            CliPrompt_clear(&cli_prompt);
        }

        sleep_ms(50);
    }
    return 0;
}
