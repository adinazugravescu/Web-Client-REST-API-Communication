# Tema 4 - Client web. Comunicaţie cu REST API.
# Zugravescu Adina Andreea 322CD

Am realizat implementarea acestei teme pornind de la Laboratorul 9, folosind scheletul sau de cod si implementarea proprie.

Clientul implementat interpreteaza corect **toate** comenzile cerute in tema si afiseaza mesaje sugestive catre utilizator. Ruleaza corect **toate** script-urile oferite, inclusiv ALL si nospace.

Am ales utilizarea bibliotecii parson (https://github.com/kgabis/parson), aceasta fiind cea recomandata si am utilizat-o in formatarea payload-ului; afisarea proprie respecta, de asemenea, formatul dorit, parsandu-se raspunsul primit si afisandu-se datele dupa modelul "proprietate": valoare, continutul fiind unul usor de procesat.

# Implementare

    Sunt declarate in main cookie-ul de sesiune si token-ul de acces in biblioteca, ce vor fi goale pana in momentul in care utilizatorul se va loga/va accesa biblioteca, caz in care vor fi setate cu informatia primita de la server. Acestea sunt instrumentele de verificare a activitatii utilizatorului pe masura ce se proceseaza comenzile:

    - `register` : se va primi de la stdin numele si parola si se va face o cerere de tip POST catre server cu informatiile de inregistrare; se va interpreta raspunsul primit de la server, verificandu-se daca exista erori, iar in caz contrar se vor reseta cookie-ul de sesiune si tokenul de acces pentru a inregistra activitatea noului utilizator inregistrat.

    - `login` : se va primi de la stdin numele si parola si se va face o cerere de tip POST catre server cu informatiile de logare; se va interpreta raspunsul primit de la server, verificandu-se daca exista erori, iar in caz contrar se va extrage din raspuns cookie-ul primit de la server pentru a dovedi logarea utilizatorului.

    - `enter_library` : se va verifica cu ajutorul cookie-ului daca utilizatorul este logat (eroare in caz contrar), caz in care se va trimite o cerere de tip GET catre server, care va returna in caz de SUCCES un token pentru a dovedi accesul sau la biblioteca.

    - `get_books` : se va verifica cu ajutorul token-ului daca utilizatorul are acces la biblioteca (eroare in caz contrar), caz in care se va trimite o cerere de tip GET catre server, care va returna in caz de SUCCES un JSON cu cartile din biblioteca ce vor fi afisate in formatul dorit.

    - `get_book` : se va primi de la stdin id-ul si se va verifica cu ajutorul token-ului daca utilizatorul are acces la biblioteca (eroare in caz contrar), caz in care se va trimite o cerere de tip GET catre server, care va returna in caz de SUCCES un JSON cu cartea dorita ce va fi afisata in formatul dorit.

    - `add_book` : se vor primi de la stdin datele cartii, verificandu-se daca acestea sunt valide (eroare in caz contrar, nu se va efectua comanda), se va verifica cu ajutorul token-ului daca utilizatorul are acces la biblioteca (eroare in caz contrar), caz in care se va trimite o cerere de tip POST catre server cu datele cartii;
    se va interpreta raspunsul primit de la server, verificandu-se daca exista erori, iar in caz contrar cartea este adaugata corect in biblioteca.

    - `delete_book` : se va primi de la stdin id-ul si se va verifica cu ajutorul token-ului daca utilizatorul are acces la biblioteca (eroare in caz contrar), caz in care se va trimite o cerere de tip DELETE catre server; se va interpreta raspunsul primit de la server, verificandu-se daca exista erori, iar in caz contrar cartea este stearsa.

    - `logout` : se va verifica cu ajutorul cookie-ului daca utilizatorul este logat (eroare in caz contrar), caz in care se va trimite o cerere de tip GET catre server, care va raspunde fie cu o eroare, iar in caz contrar se va deloga utilizatorul si se vor reseta cookie-ul de sesiune si token-ul de acces.

    - `exit` : se va opri programul fara a se mai afisa un mesaj
