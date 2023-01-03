# juliette

<p align="center">
<img src="https://github.com/alantao912/juliette/blob/main/juliette-logo.png"/>  
</p>

Computer program that plays chess. Estimated playing strength: 1600 ELO.

Features:
```
    - Bitboard state representation
    - Negamax Implicit Game Tree Traversal
    - Alpha-Beta Pruning
    - MVV-LVA move ordering
    - Static Exchange Evaluation (SEE)
    - Quiescent Search
    - Check Extensions
    - Transposition Table
    - Repetition Table
    - Incrementlly updated Zobrist Hash board indexing
    - Single Principal Variation List
    - PSQT Tables
    - Guard Heuristic
    - King Safety/Unsafety Evaluation
    - Tapered Evaluation
    - CLI, and Socket UCI interface
```

Requirements:
```
    - C++ compiler 
    - Willingness to lose
```

To compile and run, navigate to the directory of the source files and invoke the following two commands:  
```    
    - g++ *.cpp -lWS2_32 -o juliette  
    - juliette.exe cli  
```
