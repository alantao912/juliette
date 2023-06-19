# juliette

<p align="center">
<img src="https://github.com/alantao912/juliette/blob/main/juliette-logo.png" border-radius="5px"/>  
</p>

### Overview:   
*juliette* is an open source, UCI compliant **chess engine** that analyzes chess positions and determines strong moves. Currently, *juliette* has an estimated playing strength of ELO.  

*juliette* is a back-end

### Build
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