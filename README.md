YaSh:
====
YaSh is yet another Sh clone. It's my personal favorite, for obvious reasons (when it's working, that is).

Installation:
=============
Installation is very simple.

```
 $ ./autogen.sh
 $ autoconf
 $ ./configure --prefix=<prefix>
 $ make
 $ sudo make install
```

Uninstall:
==========
If you don't like the shell, you can remove it pretty easily.

```
 $ sudo make uninstall
```

Usage:
======
To run the installed shell, just type "hsh" in your terminal.

```
 $ yash
```

Line Editing:
=============
In YaSh, one can...

 * ctrl + a to go to the beginning of the line
 * ctrl + e to go to the end of a line
 * tab to do tab completion
 * ctrl + c to send a kill signal to a process
 * ctrl + d to delete a character (backspace and delete keys also work)
 * ctrl + <right arrow> to go to the next space
 * ctrl + <left arrow>  to go to the previous space. 
* use arrow keys to move through history

History:
========
 * You can do the bang-bang: !! will run the prior command.
 * !-<n> will run the think you ran n commands ago.

The only other thing to note (atm) is that you can use setenv.

In particular, if you want to change the way the terminal looks, you change
the PROMPT variable.

So, if you want to be boring, you can do the following.

```
 $ setenv PROMPT "YaSh => "
```



