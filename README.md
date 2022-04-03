# COMP304-Project1

In this project, an interactive Unix-style operating system shell called shellfyre in C/C++ is developed. 

Part 1:

Part 2:

filesearch command:
The command takes an input keyword and scans all the files in the current directory to match the keyword with the file name and then it returns the list of file(s) with this name regardless of the file extensions. When the user runs the command with -r option, command searches all the sub-directories under the current directory including the current directory.

cdh command:
The command takes no arguments. After calling cdh, shell outputs a list of most recently visited directories. The list also includes the index of the directory as both a number and an alphabetic letter. The shell then prompts the user which directory they want to navigate to; the user can select either a letter or a number from the list. After this, the shell switches to that directory.

take command:
The command takes 1 argument: the name of the directory you want to create and change into. The command creates a directory and changes path into it. The command creates the intermediate directories along the way if they do not exist. For example: if you call take A/B/C, the command creates the directories that do not exist and change into the last one (i.e, A/B/C).

joker command:
The command will generate a random joke every 15 minutes and output it to the screen.

colortext command:
The command takes a color input and returns the list of file(s) in the directory, colored in input color. When the user runs the command with -r option, command searches all the sub-directories under the current directory including the current directory and returns the file list in input color. When user enters rainbow as color input, the returned list is colored in rainbow sequence.


