

# PE32 Proxy Architecture

````mermaid
graph LR;
  A((Server.exe<br>32bit)) <--> B([PE32.dll<br>32bit]);
  A((Server.exe<br>32bit)) <==>|Shared Memory|A1;
  Proxy.dll <--> D([Application.dll<br>64bit]);
  D([Application.dll<br>64bit]) <--> E((Studio.exe<br>64bit));

  

    subgraph Proxy.dll
        direction TB
        A1((Client<br>64bit))
    end

````
