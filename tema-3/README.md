[include]: include/
[src]: src/

[main-cpp]: src/main.cpp

[cmds-hpp]: include/cmds.hpp
[cmds-cpp]: src/cmds.cpp

[Client-hpp]: include/Client.hpp
[Client-cpp]: src/Client.cpp

[rest]: https://en.wikipedia.org/wiki/Representational_state_transfer
[json]: https://en.wikipedia.org/wiki/JSON
[nlohmann-json]: https://github.com/nlohmann/json
[http]: https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol
[tcp-socket]: https://en.wikipedia.org/wiki/Network_socket
[dns]: https://en.wikipedia.org/wiki/Domain_Name_System

# Comunicarea cu un [API RESTful][rest]
Acest subdirector conține **Tema 3** din cadrul cursului _Protocoale de comunicație_ ce presupune implementarea unui client pentru comunicarea cu un [API RESTful][rest]. Programul este împărțit în mai multe componente/fișiere, fiecare din acestea rezolvând o anumită problemă.

## Structura codului sursă
În fișierele antet, aflate în [directorul include][include], se află declarațiile/definițiile de constante, structuri, funcții ce sunt folosite în cadrul fișierelor sursă. Unde este cazul, există și comentarii ce însoțesc declarațiile/definițiile menite să ușureze înțelegerea scopului acestora.

În fișierele sursă, aflate în [directorul src][src], se află implementările, fiind comentate părțile mai importante din logica programului.

## Interfața cu utilizatorul ([main.cpp][main-cpp])
Aici ne ocupăm de citirea comenzilor date de utilizator la linia de comandă și executarea codului corespunzător pentru o comandă introdusă, dacă aceasta există, prin căutarea într-un `unordered_map`.

## Procesarea comenzilor ([cmds.hpp][cmds-hpp], [cmds.cpp][cmds-cpp])
Aici se citesc de la tastatură datele introduse de către utilizator pentru comanda solicitată de acesta (e.g. `username`, `password`, `book_id`, `book_title`, `book_author` etc.), se construiește payload-ul [JSON][json], se trimite cererea către server prin intermediul unui obiect de tip `Client`, iar mai apoi se primește, se interpretează și se afișează răspunsul de la server. Pentru parsarea [JSON][json] am folosit biblioteca [JSON for modern C++][nlohmann-json] creată de Niels Lohmann.

## Implementarea clientului ([Client.hpp][Client-hpp], [Client.cpp][Client-cpp])
Un obiect `Client` este folosit pentru comunicarea cu serverul. Acesta expune public următoarele metode:
+ `get`: trimite o cerere [HTTP][http] de tipul `GET` la server
+ `post`: trimite o cerere [HTTP][http] de tipul `POST` la server
+ `del`: trimite o cerere [HTTP][http] de tipul `DELETE` la server
+ `clear`: șterge cookie-ul și token-ul de autorizare (se folosește pentru comanda `logout` din interfața cu utilizatorul)

Toate metodele menționate mai sus vor întoarce o pereche ce conține ca prim element codul de stare [HTTP][http] al răspunsului, iar ca al element secund un obiect de tipul `nlohmann::json` ce este folosit pentru afișarea unui mesaj corespunzător utilizatorului.

## Observații
+ Pentru determinarea adresei serverului se face o cerere [DNS][dns] prin intermediul `getaddrinfo`.
+ O conexiune la server se face doar atunci când aceasta este necesară.
+ Toată comunicația [HTTP][http] se efectuează manual prin [socluri TCP][tcp-socket].
+ Dacă există deja o conexiune validă la server, aceasta va fi folosită. În caz contrar, se închide conexiunea veche și se creează una nouă, toate aceste lucruri fiind gestionate în cadrul metodelor private `send` și `recv` din cadrul clasei `Client`.
+ **Nu** există verificări care se fac pe client legate de stare, astfel că utilizatorul poate de exemplu să trimită la server o cerere de `logout` chiar dacă nu este autentificat sau să poată accesa biblioteca înainte de a se autentifica. Absolut toate cererile se trimit la server, mai apoi afișându-se răspunsul primit, deci gestionarea acestor erori se face exclusiv de către server.
