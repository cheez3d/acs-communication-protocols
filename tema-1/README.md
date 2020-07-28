[include]: include/
[src]: src/

[main-c]: src/main.c

[iface-h]: include/iface.h
[iface-c]: src/iface.c

[ether-h]: include/ether.h
[ether-c]: src/ether.c

[arp-h]: include/arp.h
[arp-c]: src/arp.c
[list-h]: include/list.h
[list-c]: src/list.c

[ip-h]: include/ip.h
[ip-c]: src/ip.c

[icmp-h]: include/icmp.h
[icmp-c]: src/icmp.c

[rtable-h]: include/rtable.h
[rtable-c]: src/rtable.c
[rtable-txt]: rtable.txt

[arp]: https://en.wikipedia.org/wiki/Address_Resolution_Protocol
[mac-address]: https://en.wikipedia.org/wiki/MAC_address
[ip-address]: https://en.wikipedia.org/wiki/IP_address
[ethernet-frame]: https://en.wikipedia.org/wiki/Ethernet_frame
[ipv4]: https://en.wikipedia.org/wiki/IPv4
[icmp]: https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol
[routing-table]: https://en.wikipedia.org/wiki/Routing_table
[ttl]: https://en.wikipedia.org/wiki/Time_to_live
[checksum]: https://en.wikipedia.org/wiki/Checksum
[trie]: https://en.wikipedia.org/wiki/Trie

# Implementa procesului de dirijare a pachetelor pentru un router
Acest subdirector conține **Tema 1** din cadrul cursului _Protocoale de comunicație_ ce presupune implementarea procesului de dirijare caracteristic unui router.
Programul este împărțit în mai multe componente/fișiere, fiecare din acestea rezolvând o anumită problemă.

## Structura codului sursă
În fișierele antet, aflate în [directorul include][include], se află declarațiile/definițiile de constante, structuri, funcții ce sunt folosite predilect în cadrul fișierelor sursă. Unde este cazul, există și comentarii ce însoțesc declarațiile/definițiile menite să ușureze înțelegerea scopului acestora.
În fișierele sursă, aflate în [directorul src][src], se află implementările, fiind comentate părțile mai importante din logica programului.
În rezolvarea temei am încercat să divizez cât mai mult nivelurile separate
ale stivei, astfel că avem în cadrul implementării fișierele descrise mai jos.

## Bucla principală ([main.c][main-c])
În cadrul buclei principale primim mesajele de pe orice interfață a router-ului pe măsură ce acestea vin, le procesăm și transmitem mai departe mesaje de răspuns sau același mesaje dirijate pe calea aleasă de router. De asemenea, aici verificăm și dacă există mesaje primite de router și încă nedirijate ce se află în coada [ARP][arp] și vedem dacă le putem trimite în urma primirii unui răspuns [ARP][arp].

## Gestionarea interfețelor router-ului ([iface.h][iface-h], [iface.c][iface-c])
Aici sunt implementate funcțiile utilizate în interacțiunea directă cu interfețele router-ului, precum și funcțiile ce ne oferă posibilitatea de a obține informații legate de interfețele router-ului, precum [adresele MAC][mac-address] sau [adresele IP][ip-address],

## Gestionarea [cadrelor Ethernet][ethernet-frame] ([ether.h][ether-h], [ether.c][ether-c])
Aici se efectuează procesarea cadrelor primite. Se verifică dacă cadrele primite ne sunt destinate și de asemenea se verifică ce protocol conțin cadrele pentru a le procesa corespunzător.

## Gestionarea [protocolului ARP][arp] ([arp.h][arp-h], [arp.c][arp-c])
Aici se prcesează cererile [ARP][arp] și răspunsurile [ARP][arp] primite de router și de asemenea de aici se pot construi și cereri [ARP][arp] la nevoie.
Aici se află implementată coada [ARP] pentru mesajele de dirijat pentru care încă nu știm [adresa MAC][mac-address] corespunzătoare [adresei IP][ip-address] a următorului „hop”. Pentru coada ARP am utilizat o implementare prorpie de listă dublu-înlănțuită ([list.h][list-h], [list.c][list-c]).
Aici se află implementat de asemenea și cache-ul [ARP][arp].

## Gestionarea pachetelor [IPv4][ipv4] ([ip.h][ip-h], [ip.c][ip-c])
Aici se procesează pachetele [IPv4][ipv4] primite pentru a determina ce urmează să se întâmple cu acestea. Se verifică dacă există vreo problemă cu pachetul primit (versiune incorectă, erori de transmisie, [TTL][ttl] insuficient), generându-se un răspuns [ICMP][icmp] dacă este cazul. Se verifică dacă pachetul este destinat chiar router-ului. Dacă nu, atunci se continuă cu procesul de dirijare, în cadrul căruia se caută în tabela de dirijare după adresa destinație a pachetului. Dacă un „hop” nu este găsit, se generează un răspuns [ICMP][icmp] care se trimite înapoi la sursă. Dacă un „hop” este găsit, dar nu îi cunoaștem [adresa MAC][mac-address] corespunzătoare [adresei IP][ip-address], atunci mesajul este introdus în coada [ARP][arp] pentru a fi dirijat ulterior, după ce primim un răspuns [ARP][arp] (și răspunsurile [ICMP][icmp] generate de router sunt supuse acestui proces). În cazul în care toate informațiile sunt disponibile, pachetul este dirijat pe loc.
Actualizarea [TTL][ttl] pentru un pachet dirijat se efectuază cu ajutorul algoritmului de calculare incrementală a [sumei de control][checksum].


## Implementarea [protocolului ICMP][icmp] ([icmp.h][icmp-h], [icmp.c][icmp-c])
Aici se construiesc răspunsurile [ICMP][icmp] pe care router-ul le trimite dacă apar probleme cu pachetele [IPv4] sau în cazul în care primim o cerere [ICMP][icmp] de tip _echo_.

## Implementarea [tabelei de dirijare][routing-table] ([rtable.h][rtable-h], [rtable.c][rtable-c])
Tabla de dirijare utilizată de router se citește din fișierul [rtable.txt][rtable-txt] și se stochează în memorie într-o structură de date de tip [trie][trie] ([prefix tree][trie]), pentru o căutare mai eficientă.
