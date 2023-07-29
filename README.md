# juliette

<p align="center">
<img src="https://github.com/alantao912/juliette/blob/main/juliette-logo.png" style="border: 2px solid  gray; border-radius: 2px"/>  
</p>

### Overview:   
---
*juliette* is a strong, open source, [UCI] (https://wbec-ridderkerk.nl/html/UCIProtocol.html) compliant **chess engine** that analyzes chess positions and determines strong moves. She currently has an estimated playing strength of 1600 ELO.  

This project comes with an associated chess front-end, *romeo*, developed in ElectronJS which provides a user friendly interface for playing against *juliette*.

### Build
---
Requirements:

```
    - C++ compiler 
    - Any UCI compliant chess front end (Ex: CuteChess)
```

To compile and run, navigate to the root project directory and invoke the following two commands:

```    
    - g++ src/*.cpp -lpthread -o juliette
    - ./juliette cli
```

### About
---
