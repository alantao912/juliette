# juliette

<p align="center">
<img src="https://github.com/alantao912/juliette/blob/main/juliette-logo.png" style="width:320px;"/>  
</p>

### Overview:   
---
*juliette* is a strong, open source, [UCI](https://wbec-ridderkerk.nl/html/UCIProtocol.html) compliant **chess engine** that analyzes chess positions and determines strong moves. She currently has an estimated playing strength of 1600 ELO.  

### Build
---
Requirements:

```
    - Any UNIX derived OS
    - C++ 11 compiler or newer with support for pthread (UNIX threading library) 
    - Any UCI compliant chess front end (Ex: CuteChess)
```

To compile and run, navigate to the root project directory and invoke the following two commands:

```    
    - g++ src/*.cpp -lpthread -o juliette
    - ./juliette cli
```

### About
---

