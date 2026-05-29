#ifndef GENERATE_H
#define GENERATE_H

/*
 * PROBLEM:
 * We would like to be able to refer to elements of a C struct from assembly.
 * Specifically, where each offset within the saved_regs_t struct each register goes.
 *
 * Assembly cannot access C structs or use `offsetof` so we need to do a preprocessor step.
 *
 * SOLUTION:
 * We first create a C file that emits an object with a bunch of ASCII strings in it
 * using the .ascii feature of inline assembly.
 *
 * Each of these strings is a key value pair, where the key is the name of a struct member,
 * and the value is where to find it relative to the struct (eg. its offsetof).
 *
 * Then, we parse this file using objcopy, and then we can dump
 * the resulting file into a new header with a bunch of assembly macros that assembly can read.
 *
 * This approach is used by the Linux kernel in its kbuild system.
 * Refer to include/linux/kbuild.h in the Linux kernel
 */

/*
 * EMIT_KEY_VALUE_PAIR(key,val)
 * Emits a definition for an assembly header file of the following format:
 *
 * #define KEY VALUE
 *
 * KEY: The symbolic name of the thing to define (what you want it to be called in assembly)
 * VALUE: The value of the offset of the struct. This will be THE VALUE OF some C expression (eg. offsetof(whatever, whatever))
 *
 * For example, if the name is "REGISTER_X1", and we wanted its value to be offsetof(saved_regs_t, x1) (which is 0), then:
 * KEY is the string "REGISTER_X1"
 * VALUE is 0, which is what is returned from evaluating offsetof(saved_regs_t, x1)
 *
 * And the result would be:
 * #define REGISTER_X1 0
 *
 * The first assembly line is used to emit the ".set blah blah" thing, and
 * the second line is used to emit the new line.
 * You might be wondering why not just add a "\n" to the first .asciz?
 * Well for some reason I get the warning: "Warning: unterminated string; newline inserted"
 * and this is the easiest way to just make that go away.
 */
#define EMIT_KEY_VALUE_PAIR(key,val) \
    asm volatile( \
        "\n.string \"#define " #key " %0\"" \
        "\n.word 0x10" \
    : : "i"(val))

/*
 * EMIT_OFFSET(name,structure,member)
 * This is a higher-level function that is used to wrap around EMIT_KEY_VALUE_PAIR.
 *
 * name: The name of the thing as it should be read in assembly.
 * structure: The C struct we want to emit the offset of.
 * member: The member within that structure.
 */
#define EMIT_OFFSET(name,structure,member) \
    EMIT_KEY_VALUE_PAIR(name, offsetof(structure,member))

/*
 * BEGIN_OFFSET_DECLS
 * This is not a valid C function!
 *
 * This macro declares a pseudofunction that the compiler thinks
 * is a real C function, but in reality is actually just ASCII text data.
 *
 * Put the BEGIN_OFFSET_DECLS macro before all EMIT_OFFSET calls, and
 * throw an END_OFFSET_DECLS at the end.
 *
 * The body contents of this function are a string of key value pairs
 * that are later used to generate an assembly header file.
 *
 * This is all a massive hack to get around the fact that assembly
 * doesn't understand C structures.
 */

#ifdef ARCH_ARM
#define BEGIN_OFFSET_DECLS void __attribute__((section((".text")))) __autogen_asm_offsets(void) {
#else // ARCH_ARM
#define BEGIN_OFFSET_DECLS void __attribute__((naked))  __attribute__((section((".text")))) __autogen_asm_offsets(void) {
#endif // !ARCH_ARM

#define END_OFFSET_DECLS }

#endif // GENERATE_H
