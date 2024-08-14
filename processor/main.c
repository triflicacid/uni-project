#include <cpu.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  // parse command-line arguments
  char *file_in = NULL, *file_out = NULL;

  for (int i = 1; i < argc; i++) {
    // option?
    if (argv[i][0] == '-') {
      // next character determines type
      switch (argv[i][1]) {
        case 'o': // output file
          if (++i >= argc) {
            printf( ERROR_STR " -o: expected file path.\n");
            return EXIT_FAILURE;
          }

          file_out = argv[i];
          break;
        default:
          printf(ERROR_STR " unknown flag '%s'.\n", argv[i]);
          return EXIT_FAILURE;
      }
    } else if (file_in == NULL) {
      file_in = argv[i];
    } else {
      printf(ERROR_STR " unknown positional argument '%s'.\n", argv[i]);
      return EXIT_FAILURE;
    }
  }

  // check that an input file was provided
  if (file_in == NULL) {
    printf(ERROR_STR " expected input file as positional argument.\n");
    return EXIT_FAILURE;
  }

  // create and initialise CPU
  cpu_t cpu;
  cpu_init(&cpu);

  // was an output file specified?
  if (file_out != NULL) {
    cpu.fp_out = fopen(file_out, "w");

#if DEBUG & DEBUG_CPU
    printf(DEBUG_STR ANSI_BLUE " output file" ANSI_RESET " set to '%s' (descriptor %d)\n",
      file_out, cpu.fp_out == NULL ? -1 : fileno(cpu.fp_out));
#endif

    if (cpu.fp_out == NULL) {
      printf(ERROR_STR " -o: failed to open file '%s'.\n", file_out);
      return EXIT_FAILURE;
    }
  }

  // for exit code
  int exit_code = EXIT_SUCCESS;

  // read source file as binary
  FILE *source = fopen(file_in, "rb");

  if (source == NULL) {
    printf(ERROR_STR " failed to read source file '%s'\n", file_in);
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  // determine file size
  fseek(source, 0, SEEK_END);
  long file_size = ftell(source);
  rewind(source);

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " reading source file '%s'... %ld bytes read\n", file_in, file_size);
#endif

  // error if file size exceeds buffer size
  if (file_size >= DRAM_SIZE) {
    printf(ERROR_STR " source file size of %li bytes exceeds memory size of %d.\n", file_size, DRAM_SIZE);
    exit_code = EXIT_FAILURE;
    goto cleanup;
  }

  // get starting address
  uint64_t *start_address = cpu.regs + REG_IP;
  fread(start_address, sizeof(*start_address), 1, source);

#if DEBUG & DEBUG_CPU
  if (*start_address != 0) {
    printf(DEBUG_STR " start address specified: 0x%llx\n", *start_address);
  }
#endif

  // copy code into processor's memory
  fread(cpu.bus.dram.mem, 1, file_size - sizeof(*start_address), source);

  // start processor
  cpu_start(&cpu);

  // print_registers(&cpu);

#if DEBUG & DEBUG_CPU
  printf(DEBUG_STR " process exited with code $ret=0x%llx\n", cpu.regs[REG_RET]);
#endif

cleanup:
  // close file handles, if appropriate
  if (file_out != NULL) {
    fclose(cpu.fp_out);
  }

  if (source != NULL) {
    fclose(source);
  }

  return exit_code;
}
