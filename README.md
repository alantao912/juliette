# juliette

<p align="center">
<img src="https://github.com/alantao912/juliette/blob/main/juliette-logo.png" style="width:320px;"/>  
</p>

### Overview:   
---
*juliette* is a strong, open source, [UCI](https://wbec-ridderkerk.nl/html/UCIProtocol.html) compliant **chess engine** that analyzes chess positions and determines strong moves. She currently has an estimated playing strength of 1600 ELO.  

The standalone version of this engine only supports command line inputs. However, there is currently a work in progress of a web interface. Stay tuned!

### Build
---
Requirements:

```
    - Any UNIX derived OS
    - C++ 11 compiler or newer with support for the pthread library
    - Any UCI compliant chess front end (Ex: CuteChess)
```

To compile and run, navigate to the root project directory and invoke the following two commands:

```    
    - g++ src/*.cpp -lpthread -o juliette
    - ./juliette cli
```

### About
---

The core of her decision making process is built with an algorithm called <i>Principal Variation Search</i> (PVS); a special flavor of the classic, tried-and-true alpha beta search. Additional heuristics are used for more aggressive pruning of potentially irrelevant subtrees. These methods include razoring (WIP), futility-pruning, and delta-pruning.


High-quality evaluations of tactically quiet leaf nodes is done using a hand crafted function.