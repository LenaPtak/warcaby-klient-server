### Sprawozdanie ###

W niniejszym sprawozdaniu dotyczącym projektu semestralnego - Gra turowa warcaby są zawarte najważniejsze informacje dotyczące projektu oraz instrukcja obsługi.


# Opis protokołu

Projekt jest zrealizowany w architekturze klient-serwer z użyciem protokołu TCP z interfejsem gniazd BSD. Serwer jest napisany w języku C/C++, a klient w języku Python. Tryb komunikacji gniazd BSD jest połączeniowy. Stworzony serwer jest współbieżny, by lepiej zarządzać rozgrywkami wykorzystane są wątki. Każda rozgrywka warcab to jeden wątek.
Walidacja ruchów odbywa się po stronie serwera i tam jest też trzymany stan rozgrywki. Klient jest odpowiedzialny za GUI.


**Klient:**

1. Tworzy gniazdo o nazwie *socket1*
2. Wiąże gniazdo z adresem i portem serwera
3. Nawiązuje połączenie z serwerem
4. Odbiera potrzebne dane od servera, czyli 
    * aktualną tablicę gry ze stanem rozgrywki, 
    * kolor gracza, 
    * czyj jest ruch,
    * ewentualny komunikat do użytkownika. 
    Klient wysyła dane do serwera 
    * o zrobionym ruchu, czyli dawne i nowe współrzędne ruszonego pionka.
5. Po zakończonej rozgrywce zamykane jest gniazdo do komunikacji z serwerem.
   
**Serwer:**

1. Tworzy gniazdo
2. Związuje gniazdo z adresem i portem serwera
3. Konfiguruje kolejkę żądań gniazda
4. Obsługuje żądania od klientów w pętli:
   1. Czeka na połączenie od klienta
   2. Tworzy gniazdo do komunikacji z konkretnym klientem
   3. Odbiera i wysyła dane do klienta (wysyła: kolor gracza, czyj ruch, tablica, komunikat, odbiera: współrzędne ruchu)
   4. Zamkyka gniazdo do komunikacji z konkretnym klientem
   

# Struktura projektu

Projekt jest zbudowany jest z najważniejszych dwóch plików: server.cpp oraz client.py, między którymi następuje wymiana danych.

Na samym początku:
1. Uruchamiany jest serwer - oczekuje graczy.
2. Klienci zgłaszają się do gry.
3. Serwer ustawia klientów w kolejce i wiąże ich w pary. Każda para tworzy osobny wątek. Kiedy klient dołącza się do serwera następuje przydział kolorów pionów. Klient, który połączy się pierwszy z pary zawsze zaczyna i dostaje kolor pionków biały.
4. Serwer do klientów wysyła tablicę inicjującą oraz zaproszenie do ruchu.
5. Klient rozpoczyna grę po znalezieniu pary z drugim klientem. Rozgrywka rozpoczyna się z pierwszym ruchem pionka klienta, który połączył się pierwszy.

W trakcie rozgrywki:
1. Klient przesyła do serwera propozycje ruchu dzięki interakcji użytkownika z GUI
2. Serwer dostaje propozycje ruchu i ją waliduje, sprawdza, czy ten ruch nie zakończy gry.
3. Jeżeli ruch jest błędny klient dostaje komunikat, a jego ruch nie zostaje wprowadzony
4. Jeżeli ruch jest zgodny z zasadami serwer wysyła zaktualizowaną tablicę oraz zmianę kolejki gracza, klient aktualizuje te dane.
5. Jeżeli ruch kończy rozgrywkę obu klientom pojawia się komunikat i połączenie z serwerem zostaje zerwane.



# Sposób uruchomienia

1. Aby uruchomić projekt należy ściągnąć wszystkie pliki do jednego folderu:

   * server.cpp
   * client.py
   * wp.jpg
   * bp.jpg
   * wd.jpg
   * bd.jpg

2. Server należy skompilować poprzez kompilator g++ np. poleceniem:
   
   * g++ -pthread -o s1 server.cpp
   s1 to nazwa pliku wykonywalnego

3. Do uruchomienia pliku client.py będzie potrzebne ściągnięcie Pygame Zero. 
   Aby zrobić to na Ubuntu Linux należy wykonać serię poleceń:

    1. sudo add-apt-repository ppa:thopiekar/pygame
    2. sudo apt-get update
    3. sudo apt-get install python3-pygame
    4. pip3 install pgzero
   
    Ściąganie Pygame Zero na Windows, OSX:  [instalacja pygame](https://pygame-zero.readthedocs.io/en/1.0/installation.html)

4. By rozpocząć rozgrywkę należy uruchomić plik z serverem przez polecenie: 
   
   * ./s1
   * ./s1 *numer portu* np. "./s1 9001"

    Następnie uruchamiamy graczy poleceniem:
    * python3 client.py

    Po uruchomieniu gracza najpierw zostaniemy zapytani o adres IP oraz numer portu servera. Po ich poprawnym podaniu na ekranie pojawi się plansza. Po dołączeniu drugiego gracza rozpocznie się rozgrywka. W serverze zmienna maksymalna liczba rozgrywek jest ustawiona na 3.

    Można rozpocząć grę!


