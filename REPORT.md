# Operating Systems Assignment 02 — Report

## Student Information

- **Student Name:** Muhammad Moawiz Sipra
- **Roll Number:** BSDSF24A036
- **Repository:** `BSDSF24A036-OS-A02`
- **Course:** CMP-325 Operating Systems
- **Instructor:** Dr. Muhammad Arif Butt

---

# Feature 2: Long Listing Format (v1.1.0)

## Q1: What is the crucial difference between the `stat()` and `lstat()` system calls? In the context of the `ls` command, when is it more appropriate to use `lstat()`?

### Answer

The key difference between `stat()` and `lstat()` is how they handle symbolic links.

- **`stat()`** follows symbolic links and returns information about the file or directory to which the link points.
- **`lstat()`** does not follow symbolic links. Instead, it returns information about the symbolic link itself.

In my `ls` implementation, I used `lstat()` because I wanted to display the properties of the link itself. When running `ls -l` on a symbolic link, I need to display:

- The file type as `l` in the permissions string.
- The symbolic link's own size and permissions.
- The symbolic link's own modification timestamp.

If I had used `stat()`, a symbolic link pointing to a directory would be reported as a directory rather than as a symbolic link. This would be misleading because the user wants to see the link itself, not just its target.

Therefore, `lstat()` is the appropriate choice when implementing a long listing format that needs to identify symbolic links correctly.

## Q2: The `st_mode` field in `struct stat` contains both file type and permission bits. Explain how to use bitwise operators and predefined macros to extract this information.

### Answer

The `st_mode` field in `struct stat` stores information about a file's type, permissions, and special permission bits.

On Linux, the relevant bit groups are commonly organized as follows:

| Bits | Information |
|---|---|
| Bits 15–12 | File type |
| Bits 11–9 | Special permission bits |
| Bits 8–6 | Owner permissions |
| Bits 5–3 | Group permissions |
| Bits 2–0 | Others' permissions |

### 1. Checking the File Type

I used predefined macros such as `S_ISDIR()`, `S_ISREG()`, and `S_ISLNK()` to determine the type of a file.

For example:

```c
if (S_ISDIR(st.st_mode)) {
    printf("Directory\n");
} else if (S_ISREG(st.st_mode)) {
    printf("Regular file\n");
} else if (S_ISLNK(st.st_mode)) {
    printf("Symbolic link\n");
}
```

Conceptually, the directory-checking macro works by masking the file-type bits and comparing them with the directory type:

```c
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
```

The predefined macros make the code easier to read and reduce the need for manual bit manipulation.

### 2. Checking File Permissions

Permission bits can be checked using the bitwise AND (`&`) operator.

| Constant | Octal Mask | Meaning |
|---|---:|---|
| `S_IRUSR` | `0400` | Owner read |
| `S_IWUSR` | `0200` | Owner write |
| `S_IXUSR` | `0100` | Owner execute |
| `S_IRGRP` | `0040` | Group read |
| `S_IWGRP` | `0020` | Group write |
| `S_IXGRP` | `0010` | Group execute |
| `S_IROTH` | `0004` | Others read |
| `S_IWOTH` | `0002` | Others write |
| `S_IXOTH` | `0001` | Others execute |

For example:

```c
if (st.st_mode & S_IRUSR) {
    // The owner has read permission.
}
```

The expression `st.st_mode & S_IRUSR` checks whether the owner's read-permission bit is set.

I used these permission masks in my `format_permissions()` function to construct a permission string such as `rwxr-xr-x`.

---

# Feature 3: Column Display (v1.2.0)

## Q1: Explain the general logic for printing items in a "down then across" columnar format. Why is a simple single loop through the list of filenames insufficient?

### Answer

The **down-then-across** format displays filenames vertically within each column. After filling one column from top to bottom, the program moves to the next column.

### Implementation Logic

**Step 1: Calculate the number of columns.**

The number of columns depends on the terminal width, the maximum filename length, and the spacing between columns.

```c
num_cols = term_width / (max_filename_length + spacing);
```

The program should ensure that `num_cols` is at least one.

**Step 2: Calculate the number of rows.**

```c
num_rows = (total_files + num_cols - 1) / num_cols;
```

This formula calculates the ceiling of `total_files / num_cols` using integer arithmetic.

**Step 3: Print the filenames.**

Nested loops can be used to print the filenames in the required order:

```c
for (int row = 0; row < num_rows; row++) {
    for (int col = 0; col < num_cols; col++) {
        int index = col * num_rows + row;

        if (index < total_files) {
            printf("%s  ", files[index]);
        }
    }

    printf("\n");
}
```

The index formula `col * num_rows + row` selects filenames vertically within each column.

### Why Is a Single Loop Insufficient?

A simple loop such as:

```c
for (int i = 0; i < total_files; i++) {
    printf("%s ", files[i]);
}
```

prints filenames sequentially, producing a horizontal sequence rather than the required down-then-across arrangement.

To arrange the filenames into columns, the program must know the number of rows and calculate the appropriate index for each row and column.

Therefore, I stored the filenames in an array before calculating the layout and displaying the output.

## Q2: What is the purpose of the `ioctl()` system call in this context? What would be the limitations of using a fixed-width fallback?

### Answer

I used the `ioctl()` system call with the `TIOCGWINSZ` request to retrieve the terminal window dimensions.

Example:

```c
#include <sys/ioctl.h>
#include <unistd.h>

struct winsize w;

if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
    int term_width = w.ws_col;
}
```

### Purpose of `ioctl()`

The terminal width determines how many columns can fit on the screen without causing unnecessary line wrapping.

By obtaining the terminal width, the program can adapt its output to different terminal sizes.

### Limitations of a Fixed-Width Fallback

A fixed-width fallback, such as 80 columns, has several limitations:

1. **Wasted space:** On a terminal that is 200 columns wide, the program may not use the available space efficiently.
2. **Line wrapping:** On a terminal that is only 40 columns wide, the output may wrap onto additional lines.
3. **Poor user experience:** The output may not be properly aligned with the actual terminal dimensions.
4. **Limited adaptability:** A hard-coded width does not automatically reflect the terminal's current size.

Therefore, using `ioctl()` allows the program to create a more responsive and readable column layout. A fixed-width fallback can still be useful when terminal dimensions cannot be retrieved.

---

# Feature 4: Horizontal Column Display (v1.3.0)

## Q1: Compare the implementation complexity of "down then across" versus "across" (horizontal) printing. Which one requires more pre-calculation and why?

### Answer

The **across (horizontal)** layout prints filenames from left to right and moves to the next line when there is insufficient space.

Example logic:

```c
for (int i = 0; i < count; i++) {
    if (current_position + col_width > term_width) {
        printf("\n");
        current_position = 0;
    }

    printf("%s  ", files[i]);
    current_position += col_width;
}
```

In this approach, the program mainly needs the terminal width and an appropriate column width. The actual implementation must also account for each filename's displayed width.

### Down-Then-Across Layout

The down-then-across layout requires more calculations:

- Determine the total number of filenames.
- Calculate the number of columns that fit in the terminal.
- Calculate the number of rows.
- Use nested loops to traverse rows and columns.
- Calculate the array index using `col * num_rows + row`.

### Which Requires More Pre-Calculation?

The **down-then-across layout** generally requires more pre-calculation because it must determine the grid dimensions before printing the filenames in the correct order.

The horizontal layout can process filenames sequentially, making its ordering logic simpler.

Both layouts may need to store filenames in memory, particularly when the program needs to calculate maximum filename widths or sort the entries before displaying them.

## Q2: Describe the strategy used to manage different display modes. How did your program decide which function to call?

### Answer

I used integer flag variables to track the selected display mode.

For example:

```c
int long_format = 0;
int horizontal_flag = 0;
int recursive_flag = 0;
```

In the `main()` function, I used `getopt()` to parse command-line options:

```c
int opt;

while ((opt = getopt(argc, argv, "lxR")) != -1) {
    if (opt == 'l') {
        long_format = 1;
    } else if (opt == 'x') {
        horizontal_flag = 1;
    } else if (opt == 'R') {
        recursive_flag = 1;
    }
}
```

After parsing the options, I passed the appropriate flags to the directory-listing function.

The program then selected the relevant display function:

```c
if (long_format) {
    display_long(...);
} else if (horizontal_flag) {
    display_horizontal(...);
} else {
    display_columns(...);  // Default format
}
```

The ellipses (`...`) represent the arguments required by the actual functions.

### Why Use This Approach?

This approach separates option parsing from output formatting.

- `-l` selects the long listing format.
- `-x` selects horizontal display.
- `-R` enables recursive directory listing.
- If no display option is selected, the default column display is used.

Keeping the display functions separate makes the program easier to understand, maintain, test, and extend.

---

# Feature 5: Alphabetical Sort (v1.4.0)

## Q1: Why is it necessary to read all directory entries into memory before sorting? What are the potential drawbacks of this approach for directories containing millions of files?

### Answer

The `readdir()` function returns directory entries in filesystem-dependent order. It does not guarantee alphabetical ordering.

To display filenames alphabetically, I first collected the entries into an array and then sorted them using `qsort()`.

### Why Read All Entries First?

The process follows these steps:

1. Read directory entries using `readdir()`.
2. Store the filenames in an array.
3. Sort the array using `qsort()`.
4. Display the sorted filenames.

For example:

```c
while ((entry = readdir(dir)) != NULL) {
    // Store each filename in the array.
}

qsort(files, count, sizeof(char *), compare_strings);
```

The array must contain the elements to be sorted before the sorting operation can organize them.

### Drawbacks for Very Large Directories

**1. High memory usage**

Storing millions of filenames and their associated data in memory can consume a significant amount of RAM.

**2. Delayed output**

The program generally needs to read all entries and finish sorting before displaying the sorted results.

**3. Fixed array limits**

If the implementation uses a fixed-size array, such as:

```c
#define MAX_FILES 10000
```

then directories containing more than 10,000 entries may exceed the array's capacity unless the program handles the limit correctly.

**4. Sorting overhead**

Comparison-based sorting, such as the usual implementation of `qsort()`, has average time complexity of \(O(n \log n)\). Sorting millions of entries can therefore take considerable time.

### Possible Improvements

For large directories, dynamic memory allocation using `malloc()` and `realloc()` can help avoid a fixed entry limit. For extremely large datasets, an external sorting approach may be appropriate.

## Q2: Explain the purpose and signature of the comparison function required by `qsort()`. How does it work, and why must it take `const void *` arguments?

### Answer

The `qsort()` function sorts an array using a comparison function supplied by the programmer.

The comparison function has the following signature:

```c
int compare(const void *a, const void *b);
```

### Purpose of the Comparison Function

The comparison function tells `qsort()` how to order two elements.

It must return:

- A negative value if the first element should appear before the second.
- Zero if the elements are considered equal.
- A positive value if the first element should appear after the second.

### Example Implementation

For an array of strings stored as `char *`, the comparison function can be written as follows:

```c
#include <stdlib.h>
#include <string.h>

int compare_strings(const void *a, const void *b) {
    const char *str1 = *(const char * const *)a;
    const char *str2 = *(const char * const *)b;

    return strcmp(str1, str2);
}
```

The array can then be sorted with:

```c
qsort(files, count, sizeof(char *), compare_strings);
```

### Why Use `const void *`?

**1. `void *` supports generic sorting**

The `qsort()` function is designed to work with arrays of different data types, including integers, strings, and structures.

Using `const void *` allows the comparison function to accept pointers to elements without requiring a separate function signature for each type.

**2. `const` prevents modification through the arguments**

The `const` qualifier indicates that the comparison function should read the elements rather than modify them through these pointers.

**3. Casting and dereferencing**

Because the array contains `char *` elements, each argument points to an array element of type `char *`.

The expression:

```c
*(const char * const *)a
```

interprets the argument as a pointer to a string-pointer element and retrieves the string pointer.

Finally, `strcmp()` compares the two strings alphabetically according to the character values used by the C library.

---

# Feature 6: Colorized Output (v1.5.0)

## Q1: How do ANSI escape codes work to produce color in a standard Linux terminal? Show the specific code sequence for printing text in green.

### Answer

ANSI escape codes are special character sequences that compatible terminals interpret as formatting commands.

An escape sequence typically begins with the ESC character, represented in C as `\033` or `\x1B`.

### General Format

```text
\033[<code>m
```

The code specifies the desired text style or color.

### Printing Green Text

The ANSI sequence for normal green foreground text is:

```text
\033[0;32m
```

Its components are:

- `\033`: The ESC character.
- `[`: Begins the control sequence.
- `0`: Resets the text style to normal.
- `32`: Selects green as the foreground color.
- `m`: Ends the Select Graphic Rendition (SGR) command.

To restore the terminal's default text style and color, use:

```text
\033[0m
```

### Example in C

```c
#include <stdio.h>

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RESET "\033[0m"

int main(void) {
    printf("%sGreen Text%s\n", COLOR_GREEN, COLOR_RESET);
    return 0;
}
```

The terminal displays `Green Text` in green and then restores the default formatting.

In my implementation, I used color macros to make the output formatting easier to maintain.

## Q2: To color an executable file, you need to check its permission bits. Explain which bits in the `st_mode` field you need to check to determine if a file is executable by the owner, group, or others.

### Answer

To determine whether a file has an execute permission, I check three permission bits in the `st_mode` field.

| Constant | Octal Mask | Meaning |
|---|---:|---|
| `S_IXUSR` | `0100` | Owner execute |
| `S_IXGRP` | `0010` | Group execute |
| `S_IXOTH` | `0001` | Others execute |

### Example Implementation

```c
if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
    return COLOR_GREEN;
}
```

### How Does It Work?

The bitwise OR (`|`) operator combines the three execute-permission masks into one mask.

The bitwise AND (`&`) operator then checks whether any of those bits is set in `st->st_mode`.

If at least one execute bit is set, the condition evaluates to true, and the function returns the green color code.

A file does not need to be executable by all three permission classes. Having an execute bit set for the owner, group, or others is sufficient for this test.

**Note:** This test identifies files with at least one execute permission bit. It does not guarantee that the current user can execute the file, because actual access also depends on ownership, group membership, other permission checks, and filesystem conditions.

---

# Feature 7: Recursive Listing (v1.6.0)

## Q1: In a recursive function, what is a "base case"? In the context of your recursive `ls`, what is the base case that stops the recursion from continuing forever?

### Answer

A **base case** is a condition that allows a recursive function to finish without making further recursive calls.

Without a proper stopping condition, a recursive function could continue calling itself until the program runs out of stack space.

### Base Case in Recursive `ls`

In my recursive directory-listing implementation, the function processes the entries in the current directory and recursively lists eligible subdirectories.

For example, the general logic is:

1. Open the current directory.
2. Read its entries.
3. Identify eligible subdirectories.
4. Construct their paths and recursively list them.
5. Finish processing the current directory and return.

The recursion naturally stops when a directory contains no eligible subdirectories to visit. The function processes the entries, makes no additional recursive calls, and returns.

More precisely, the end of the directory-entry loop is not necessarily an explicit base-case condition. Instead, it is the point where the current invocation finishes because no further recursive calls are required.

### Important Consideration

The implementation must also avoid recursively entering `.` and `..`, because they refer to the current directory and its parent directory. Following these entries would cause repeated traversal and potentially infinite recursion.

## Q2: Explain why it is essential to construct a full path (e.g., `parent_dir/subdir`) before making a recursive call. What would happen if you simply called `do_ls("subdir")` from within the `do_ls("parent_dir")` function call?

### Answer

Constructing the correct path is essential because `opendir()` interprets a relative path with respect to the process's current working directory (CWD), not automatically with respect to the directory being processed by the current recursive function call.

Suppose the program is processing:

```text
/home/user
```

and discovers a subdirectory named `docs`.

The correct recursive call is:

```c
do_ls("/home/user/docs", ...);
```

Calling the function like this is potentially incorrect:

```c
do_ls("docs", ...);
```

### What Happens If We Pass Only `"docs"`?

The operating system interprets `"docs"` relative to the process's current working directory.

If the CWD is not `/home/user`, `opendir("docs")` may fail because the expected directory cannot be found there. Alternatively, the program might open a different directory with the same name.

This makes the program unreliable when invoked from different locations.

### Constructing the Path with `snprintf()`

I used `snprintf()` to construct the child directory's path before making the recursive call.

Example:

```c
char fullpath[1024];

snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, files[i]);

do_ls(fullpath, ...);
```

Here, `dir` represents the parent directory path, and `files[i]` represents the child entry's name.

`snprintf()` helps prevent writing beyond the destination buffer. However, the program should also check whether the resulting path was truncated and handle paths that exceed the buffer capacity.

By constructing the child path correctly, the program can traverse the directory hierarchy without depending on the current working directory.

---

# Conclusion

Through this Operating Systems Assignment, I gained practical experience with Linux system calls, file metadata, directory traversal, sorting, terminal formatting, and recursive programming.

The major concepts and techniques covered in the assignment include:

1. **System Calls:** Using `stat()`, `lstat()`, `opendir()`, `readdir()`, and `ioctl()` for file metadata, directory operations, and terminal dimensions.
2. **Data Structures:** Storing directory entries in arrays for sorting and formatted output.
3. **Sorting:** Using `qsort()` with a custom comparison function to arrange filenames alphabetically.
4. **Bit Manipulation:** Checking file types and permission bits through `st_mode`, predefined macros, and bitwise operators.
5. **Command-Line Parsing:** Using `getopt()` to process options such as `-l`, `-x`, and `-R`.
6. **Output Formatting:** Implementing long listing, down-then-across columns, and horizontal column display.
7. **ANSI Colors:** Using terminal escape sequences to color filenames based on their properties.
8. **Recursion:** Traversing directory trees while constructing the correct paths and avoiding repeated traversal of `.` and `..`.
9. **Build Automation:** Using a Makefile to compile the project.
10. **Git Workflow:** Managing feature branches, version tags, and GitHub releases.

The assignment provided an opportunity to apply Operating Systems concepts in a practical C programming project. Each feature builds on the previous implementation, improving the functionality and usability of the custom `ls` utility.

The project was developed through successive versions, from `v1.0.0` to `v1.6.0`, covering the seven features of PA-02.

