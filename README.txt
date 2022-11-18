// Boldeanu Ana-Maria
// APD - Tema 1 - Paradigma Map-Reduce

=============================== Descrierea solutiei ===============================

    * main.cpp

    Thread-ul principal se ocupa intai de citirea datelor de intrare si crearea unei
liste de fisiere ce urmeaza a fi procesate (list<pair<string, int>> data_files).
Aceasta lista contine perechi de <nume, dimensiune> si este sortata descrescator, 
in functie de dimensiunea fisierelor.
    
    Fiecare Mapper va primi adresa listei si va accesa cate un fisier pe rand,
alocarea fisierelor catre thread-uri fiind astfel dinamica. 
    Am folosit un mutex pentru a sincroniza accesul Mapperilor la lista atunci 
cand acestia vor sa extraga urmatorul fisier de procesat (zona critica consta in
accesul elementului si eliminarea sa din lista, actiune ce se poate executa de 
un singur thread la un moment dat). Mai departe, Mapperii isi continua lucrul in 
paralel.

    In thread-ul principal am alocat si o matrice de liste partiale, a carei adresa
va fi primita de catre toate thread-urile in argument (list<int> ***mapper_results).
Aceasta matrice poate fi interpretata drept
    results[mapper_ID][exponent] <=> list_of_values*
unde
    mapper_ID = ID-ul Mapperului care se va ocupa de randul respectiv din matrice
    exponent = fiecare exponent ce trebuie procesat din {0 .. <numar_Reduceri> + 2}
    list_of_values* = pointer la lista partiala de valori gasite ca puteri perfecte 
        de catre Mapperul <ID> pentru exponentul respectiv

    Astfel, fiecare Mapper va lucra pe un rand separat in cadrul acestei matrice,
construind liste partiale pentru fiecare exponent fara sa suprascrie alte liste.

    Reducerii vor astepta ca toti Mapperii sa fi terminat procesarea listelor inainte
sa le compuna. Fiecare Reducer se va ocupa de cate o coloana din matrice, garantand
lucrul in paralel fara suprascrieri.


    * threads.h

    Am definit 2 structuri folosite pentru pasarea argumentelor catre thread-uri,
struct mapper_data si reducer_data. Este nevoie de:
- pointer catre matricea de liste partiale (vezi rationamentul mai sus);
- thread ID, ca fiecare thread sa acceseze doar randul/coloana asignate;
- pointer la un mutex, doar pentru Mapperi (vezi rationamentul mai sus);
- pointer la lista de fisiere ce trebuie procesate, doar pentru Mapperi;
- pointer la o bariera, comuna pentru Mapperi si Reduceri; Mapperii se vor strange 
chiar inainte de sfarsitul executiei (cand lista de fisiere de procesat s-a epuizat),
iar Reducerii se vor strange inainte sa inceapa procesarea rezultatelor de la Mapperi
(deci bariera va trebui sa astepte <numar_Mapperi> + <numar_Reduceri> thread-uri).

    Mapperii extrag fisiere din lista si le parcurg, atata timp cat inca mai sunt
fisiere de procesat.

    Pentru fiecare exponent, verificarea de puteri perfecte se face in felul urmator:
(vezi utils.h)
    Similar cu cautarea binara, functia findNthPowerBase(int value, int exp, bool &found)
cauta o baza X astfel incat X ^ exp = value. Intai se incearca gasirea bazei ca o 
valoare de mijloc intre 0 si value, ridicata la puterea exp:
- daca rezultatul este mai mare decat value, se incearca intre 0 si baza anterioara;
- altfel, se incearca intre baza anterioara si value.
    Functia intoarce valoarea de mijloc, mid, in momentul indeplinirii conditiei 
mid ^ exp = value (insa baza nu este ceruta de enunt, fiind nevoie doar de 
confirmarea ca value este putere perfecta).
    
    Pentru calculul puterii unui intreg, am implementat functia power(int value, int power, 
bool &overflow). Aceasta verifica si daca a avut loc overflow, caz tratat separat in
cadrul functiei findNthPowerBase.

    Reducerii concateneaza toate listele partiale pentru exponentul asignat, apoi le 
sorteaza pentru a pastra doar valorile unice.
    Pentru concatenarea, sortarea si filtrarea listelor si numararea elementelor unice
am folosit functii specifice definite in std::list.

=======================================================================================