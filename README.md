# custom-shell-in-c

A lightweight, UNIX-style command-line shell implemented in C, replicating essential shell functionalities with advanced process management.

## 🚀 Features

- ✅ Command execution of external programs
- ✅ Built-in commands: `cd`, `exit`, `jobs`, `fg`, `bg`
- ✅ Multi-stage pipelines (e.g., `ls | grep txt | sort`)
- ✅ Input and output redirection (`>`, `<`)
- ✅ Background execution with `&`
- ✅ Job control: list, stop, resume, foreground jobs
- ✅ Signal handling (Ctrl+C, Ctrl+Z)

## 📚 Technical Stack

- **Language**: C
- **Libraries**: POSIX system calls

## ⚙️ How It Works

- Forks and execs commands in child processes
- Handles pipes with `pipe()` and `dup2()`
- Manages redirection of standard I/O
- Tracks background jobs with process groups
- Implements `jobs`, `fg`, `bg` using process IDs and signals
- Handles user interrupts gracefully with custom signal handlers

## 💡 Why This Project?

This project demonstrates strong understanding of operating system concepts:

- Process creation and management
- Inter-process communication
- Signal handling
- Terminal control
- System-level I/O

It’s designed to showcase system programming skills often asked about in interviews at companies like Google, Amazon, Microsoft, and other product-based tech companies.

## 🛠️ Build and Run

Compile:

```bash
gcc -o mysh myshell.c
Run:
./mysh
🧪 Example Usage
mysh> ls -l
mysh> echo "Hello World" > file.txt
mysh> cat < file.txt
mysh> ls | grep .c | sort
mysh> sleep 30 &
mysh> jobs
mysh> fg 1
mysh> bg 1
📜 License
MIT License

✅ After pasting it, save in nano:
Ctrl + O
Enter
Ctrl + X
