# ConsistencyChecker

A tool for obtaining all possible outcomes of a program under two consistency models IBM370 and x86-TSO.

Used in the paper: Alberto Ros and Stefanos Kaxiras, "Speculative Enforcement of Store Atomicity", MICRO 2020. 

An example of program for the checker is given (file n6.in).

The checker reads the program file by the standard input: ./consistency-checker < n6.in

The output for this example is:

```
PROGRAM LOADED:  
st x, 1		st y, 2  
ld x		st x, 2  
ld y					

IBM370 (STORE-ATOMIC) POSSIBLE SOLUTIONS:
[x]==1; [y]==2; x==1; y==2; 
[x]==2; [y]==2; x==1; y==0; 
[x]==2; [y]==2; x==1; y==2; 
[x]==2; [y]==2; x==2; y==2; 

TSO (WRITE-ATOMIC) POSSIBLE SOLUTIONS (* breaks store atomicity):
[x]==1; [y]==2; x==1; y==0; *
[x]==1; [y]==2; x==1; y==2; 
[x]==2; [y]==2; x==1; y==0; 
[x]==2; [y]==2; x==1; y==2; 
[x]==2; [y]==2; x==2; y==2; 
```

