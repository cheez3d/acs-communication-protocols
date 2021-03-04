[include]: include/
[src]: src/
[src-subscriber]: src/subscriber/
[src-server]: src/server/

[msg-hpp]: include/msg.hpp

[utils-hpp]: include/utils.hpp

[client-main-cpp]: src/subscriber/main.cpp
[server-main-cpp]: src/server/main.cpp

[tcp]: https://en.wikipedia.org/wiki/Transmission_Control_Protocol
[udp]: https://en.wikipedia.org/wiki/User_Datagram_Protocol

# Aplicație client-server [TCP][tcp] și [UDP][udp] pentru gestionarea mesajelor
Acest subdirector conține **Tema 2** din cadrul cursului _Protocoale de comunicație_ ce presupune implementarea unei aplicații client-server ce utilizează atât [protocolul TCP][tcp], cât și [protocolul UDP][udp] pentru transmiterea de mesaje. Programul este împărțit în mai multe componente/fișiere, fiecare din acestea rezolvând o anumită problemă. S-a încercat o implementare a temei într-un mod cât mai eficient cu putință. Astfel, pentru toate mesajele trimise ori de către client, ori de către server, se vor trimite exact atâția octeți cât sunt necesari pentru a transmite complet informația și nimic mai mult.

## Structura codului sursă
În fișierele antet, aflate în [directorul include][include], se află declarațiile/definițiile de constante, structuri, funcții ajutătoare ce sunt folosite în cadrul fișierelor sursă. Unde este cazul, există și comentarii ce însoțesc declarațiile/definițiile menite să ușureze înțelegerea scopului acestora.

În fișierele sursă, aflate în [directorul src][src], se află implementările, fiind comentate părțile mai importante din logica programelor client și server. Sursele aferente clientului se află în [directorul subscriber][src-subscriber], iar cele aferente serverului se află în [directorul server][src-server], în cazul de față doar fișierele `main.cpp`.

## Structurile de date folosite în cadrul protocoalelor ([msg.hpp][msg-hpp])
+ Structura `msg_dgram` este utilizată pentru interpretarea mesajelor primite de la clienții [UDP][udp].
+ Structura `msg_stream` este utilizată pentru construirea/interpretarea mesajleor ce se transmit între clienții [TCP][tcp] și server. Tipurile de mesaje `msg_stream` disponibile sunt următoarele:
  + `NAME`: un client [TCP][tcp] ce tocmai s-a conectat și-a trimis numele către server
  + `SUBSCRIBE`: un client [TCP][tcp] a trimis o cerere de abonare către server
  + `UNSUBSCRIBE`: un client [TCP][tcp] a trimis o cerere de dezabonare către server
  + `RESPONSE`: serverul a răspuns cererii primite de la un client [TCP][tcp] cu un mesaj
  + `DATA`: serverul a dirijat un mesaj primit de la un client [UDP][udp] către un client [TCP][tcp]
+ Pentru folosirea memoriei în mod eficient am utilizat un `union` care are aceeași dimensiune cu un mesaj de tip `msg_stream` și care include în mesajul `msg_stream` și un mesaj de tip `msg_dgram`, pentru a se face cu ușurință transmiterea mesajelor primite de la clienții [UDP][udp] la clienții [TCP][tcp] corespunzători, atunci când aceștia sunt conectați la server și abonați la topic-uri. Partea `buf` din acest `union` este folosită și ca buffer pentru citirea comenzilor primite de la tastatură.

## Elemente ajutătoare ([utils.hpp][utils-hpp])
Aici se rețin elementele ajutătoare, cum ar fi șirurile de caractere corespunzătoare comenzilor de abonare/dezabonare și ieșire, funcțiile ajutătoare de procesare a argumentelor din linia de comandă etc.

## Client ([main.cpp][client-main-cpp])
Funcția clientului este aceea de a primi mesajele destinate acestuia de la server pentru topic-urile la care acesta este abonat, și, de asemenea, trimiterea către server a intențiilor de abonare/dezabonare la/de la un topic.

## Server ([main.cpp][server-main-cpp])
Funcția serverului este aceea de a gestiona transferul de mesaje între clienții [TCP][tcp] și clienții [UDP][udp] și de a gestiona cererile de abonare/dezabonare ale clienților [UDP][udp].

### Store & Forward
Pentru componenta _Store & Forward_ a serverului m-am folosit de containerele puse la dispoziție de către STL, și anume `unordered_map` și `list`. În momentul în care un client [UDP][udp] trimite un mesaj la care este abonat un client [TCP][tcp] cu `SF=1`, dar care momentan este deconectat, mesajul primit de la clientul [UDP][udp] se salvează de către server în `sfs` pentru ca mai apoi, când clientul [TCP][tcp] se reconectează, să fie trimis către acesta.

### Tratarea cazurilor speciale
Pentru tratarea cazurilor speciale de tipul „_un client [TCP][tcp] încearcă să se aboneze la un topic la care deja este abonat_”, „_un client [TCP][tcp] încearcă să se dezaboneze de la un topic la care nu e abonat_”, „_un client [TCP][tcp] încearcă să se conecteze la server cu un ID deja utilizat_” se va transmite la clientul [TCP][tcp] în cauză un mesaj `msg_stream` de tip `RESPONSE` care va conține în acesta un mesaj pe care îl va afișa utilizatorului cu eroarea survenită în cadrul comenzii trimise de client inițial.
