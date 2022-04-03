# COMP304-Project1

In this project, an interactive Unix-style operating system shell called shellfyre in C/C++ is developed. 

Part 1:
Basic commands are used by executing the relative file in /bin/ by utilizing path resolving. & function could not be implemented. 
Part 2:

filesearch command:
The command takes an input keyword and scans all the files in the current directory to match the keyword with the file name and then it returns the list of file(s) with this name regardless of the file extensions. When the user runs the command with -r option, command searches all the sub-directories under the current directory including the current directory.

cdh command:
The command takes no arguments. After calling cdh, shell outputs a list of most recently visited directories. The list also includes the index of the directory as both a number and an alphabetic letter. The shell then prompts the user which directory they want to navigate to; the user can select either a letter or a number from the list. After this, the shell switches to that directory. Uses a dir_hist.txt file to keep the history of cd. Needs be deleted to free memory from time to time.

take command:
The command takes 1 argument: the name of the directory you want to create and change into. The command creates a directory and changes path into it. The command creates the intermediate directories along the way if they do not exist. For example: if you call take A/B/C, the command creates the directories that do not exist and change into the last one (i.e, A/B/C).

joker command:
The command will generate a random joke every 15 minutes and output it to the screen.

colortext command:
The command takes a color input and returns the list of file(s) in the directory, colored in input color. When the user runs the command with -r option, command searches all the sub-directories under the current directory including the current directory and returns the file list in input color. When user enters rainbow as color input, the returned list is colored in rainbow sequence. This command is implemented by Ayten Dilara Yavuz, ID: 72281.

wfile command:
Takes two arguments: file name and string. Writes the second argument into the file with the file name (first argument). If the file doesn't exist, creates the file and writes the contents (second argument). May cause segmentation faults sometimes. I couldnt figure out why. Implemented by Ahmet Basar Erturk

Resources:
https://superuser.com/questions/299694/is-there-a-directory-history-for-bash
https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c -> used in colortext command
https://stackoverflow.com/questions/8579330/appending-to-crontab-with-a-shell-script-on-ubuntu -> joker command
https://stackoverflow.com/questions/16519673/cron-with-notify-send -> joker command
https://pubs.opengroup.org/onlinepubs/7908799/xsh/dirent.h.html  -> filesearch command
https://c-for-dummies.com/blog/?p=3246 -> filesearch command
https://stackoverflow.com/questions/12489/how-do-you-get-a-directory-listing-in-c -> filesearch command
