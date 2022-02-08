"""
Lena Ptak 145226
Klient w języku Python.
Jest odpowiedzialny za przyjmowanie inputu od użytkownika i servera, pokazywanie aktualnego stanu gry przez GUI.
"""
import pygame as p
import socket, pickle, time
from socket import AF_INET, SOCK_STREAM


#HOST = '127.0.0.1'  # Standard loopback interface address (localhost)
#PORT = 9001        # Port to listen on (non-privileged ports are > 1023)

print('Podaj dres ip:')
HOST = str(input())
print('Podaj numer portu:')
PORT = int(input())

#Dane do rysowania tablicy
WIDTH = HEIGHT = 512  # Można też dać 400
DIMENSION = 8  # Wymiary tablicy 8x8
SQ_SIZE = HEIGHT // DIMENSION
MAX_FPS = 15
IMAGES = {}

# --- FUNKCJE GLOBALNE --------------------------------------------------------------------------------------------------  

"""
Łączenie z serverem
"""
def polaczServer():
    socket1 = socket.socket(AF_INET, SOCK_STREAM)
    socket1.connect((HOST, PORT))


"""
Komunikat do servera
"""
def wyslijServerowi(desc, wiadomosc, kolor_gracza):
    wiadomosc = konwertujOutput(wiadomosc, kolor_gracza)
    b = wiadomosc.encode('utf-8')
    desc.send(b)
    return b


def odbierzServer(desc):
    return desc.recv(2000)


'''
Konwertujemy input od servera
'''
def konwertujInput(input):
    #konwertowanie z bajtow na string
    new_input = input.decode('utf-8')

    osobno = new_input.split(";")

    kolor_gracza = osobno[0]
    kogo_ruch = osobno[1]
    cala_tablica = osobno[2]
    komunikatWHITE = osobno[3]
    komunikatBLACK = osobno[4]

    tab1 = cala_tablica.split(".")
    tab = []
    for i in range(8):
        tym_tab = tab1[i].split(",")
        tab.append(tym_tab)

    return [komunikatWHITE, komunikatBLACK, kolor_gracza, tab, kogo_ruch]

'''
Konwertujemy output od klienta
'''
def konwertujOutput(output, kolor_gracza):
    #wiadomosc ma na początku postać tablicy: [(x1, y1), (x2, y2)], a ma mieć postać "white;1,2;3,4"
    odp =  [kolor_gracza, str(output[0][0]), str(output[0][1]), str(output[1][0]), str(output[1][1])]
    final_odp = ';'.join(odp)

    return final_odp



'''
Inicjalizacja globalnego słownika zdjęć pionków. Wywołujemy tą funkjcę raz w main.
'''
def loadImages():
    pieces = ['wp', 'bp']
    for piece in pieces:
        IMAGES[piece] = p.transform.scale(p.image.load(piece + ".png"), (SQ_SIZE, SQ_SIZE))


def rysujPlansze(screen, gs, kom, nick, komunikat_ruch, clock):
    drawGameState(screen, gs)
    screen.blit(kom, (0, 0))        #Tekst      
    screen.blit(nick, (0, 20))
    screen.blit(komunikat_ruch, (0, 40))
    clock.tick(MAX_FPS)
    p.display.flip()


'''
Funkcja odpowiedzialna za wszelką grafikę w grze.
'''
def drawGameState(screen, gs):
    drawBoard(screen)  # rysuje kwadraty na planszy
    drawPieces(screen, gs.board)  # rysuje pionki


'''
Rysuje kwadraty na planszy.
'''
def drawBoard(screen):
    colors = [p.Color("white"), p.Color("light blue")]
    for r in range(DIMENSION):
        for c in range(DIMENSION):
            color = colors[((r + c) % 2)]
            p.draw.rect(screen, color, p.Rect(c * SQ_SIZE, r * SQ_SIZE, SQ_SIZE, SQ_SIZE))


'''
Rysuje pionki na planszy używuająć obecnego GameState.board
'''
def drawPieces(screen, board):
    for r in range(DIMENSION):
        for c in range(DIMENSION):
            piece = board[r][c]
            if (piece != "--") and (piece != "xx"):  # niepuste pole
                screen.blit(IMAGES[piece], p.Rect(c * SQ_SIZE, r * SQ_SIZE, SQ_SIZE, SQ_SIZE))



#---- GAME STATE -------------------------------------------------------------------------------------------------------------

"""
Tu będziemy trzymać wszystkie informacje o obecnym stanie gry.
"""
class GameState():
    def __init__(self):
        # tablica 8x8, każdy element listy ma 2 znaki np. 
        #"--" - puste pole
        #"xx" - niedostępne pole 
        #"bp" - pionek biały 
        #"wp" - pionek czarny
        self.board = [
            ["xx", "wp", "xx", "wp", "xx", "wp", "xx", "wp"],
            ["wp", "xx", "wp", "xx", "wp", "xx", "wp", "xx"],
            ["xx", "wp", "xx", "wp", "xx", "wp", "xx", "wp"],
            ["--", "--", "--", "--", "--", "--", "--", "--"],
            ["--", "--", "--", "--", "--", "--", "--", "--"],
            ["bp", "xx", "bp", "xx", "bp", "xx", "bp", "xx"],
            ["xx", "bp", "xx", "bp", "xx", "bp", "xx", "bp"],
            ["bp", "xx", "bp", "xx", "bp", "xx", "bp", "xx"]
        ]
        self.whiteToMove = True  #kto robi ruch
        self.moveLog = []        #tablica ruchów

    """
    Bierze Move jako parametr i wyknuje go (nie będzie zawsze działać)
    """
    def makeMove(self, move, tab):
        #jezeli mamy juz tablice od servera
        self.board = tab

        #bez tablicy bylo tak
        #self.board[move.startRow][move.startCol] = "--"
        #self.board[move.endRow][move.endCol] = move.pieceMoved
        self.moveLog.append(move)  # tu zapisujemy ruchy, żeby cofnąć w razie co
        self.whiteToMove = not self.whiteToMove  # zmień gracza


#---- MOVE ------------------------------------------------------------------------------------------------------------

class Move():
    # mapuje klucze do wartości
    # klucz : wartość
    ranksToRows = {"1": 7, "2": 6, "3": 5, "4": 4,
                   "5": 3, "6": 2, "7": 1, "8": 0}
    rowsToRanks = {v: k for k, v in ranksToRows.items()}
    filesTocols = {"a": 0, "b": 1, "c": 2, "d": 3,
                   "e": 4, "f": 5, "g": 6, "h": 7}
    colsToFiles = {v: k for k, v in filesTocols.items()}

    # info o ruchach
    def __init__(self, startSq, endSq, board):
        self.startRow = startSq[0]
        self.startCol = startSq[1]
        self.endRow = endSq[0]
        self.endCol = endSq[1]
        self.pieceMoved = board[self.startRow][self.startCol]
        self.pieceCaptured = board[self.endRow][self.endCol]

    def getChessNotation(self):
        return self.getRanksFile(self.startRow, self.startCol) + self.getRanksFile(self.endRow, self.endCol)

    def getRanksFile(self, r, c):
        return self.colsToFiles[c] + self.rowsToRanks[r]


# --- GLOWNA PETLA -----------------------------------------------------------------------------------------------------------

def main():

    licznik = 0     #pomocniczy

    # --- OBSLUGA SERVERA ----------------------------------------------------------------------------------------------------

    #Połączenie się z serverem.
    socket1 = socket.socket(AF_INET, SOCK_STREAM)
    socket1.connect((HOST, PORT))
    socket1.settimeout(300)


    #Wiadomość próbna pierwsza
    socket1.send(b'white;1,2;3,4')


    #Odbieramy pierwszą tablicę od servera.
    inputPRZED = odbierzServer(socket1)
    kolor_gracza = konwertujInput(inputPRZED)[2]
    tab_konw = konwertujInput(inputPRZED)[3]
    kogo_ruch = konwertujInput(inputPRZED)[4]
    if kolor_gracza == "white":
        komunikat = konwertujInput(inputPRZED)[0]
    else:
        komunikat = konwertujInput(inputPRZED)[1]


    licznik += 1
    print(licznik, "\nOdebrałem na starcie: \n")
    for i in range(8):
        print(tab_konw[i])
    print("\nKomunikat: ", komunikat)
    print("Czyj ruch: ", kogo_ruch)
    print("Kolor gracza: ", kolor_gracza, "\n")


    time.sleep(3)


    # ----- INICJACJA GRY I PLANSZY ----------------------------------------------------------------------------------------------------

    p.init()
    myfont = p.font.SysFont('Comic Sans MS', 30)
    kom = myfont.render(komunikat, True, (0, 0, 0))
    nick = myfont.render(''.join(["Twój kolor to ", kolor_gracza]), True, (0, 0, 0))
    komunikat_ruch = myfont.render(''.join(["Teraz ruch ma ", kogo_ruch]), True, (0, 0, 0))
    screen = p.display.set_mode((WIDTH, HEIGHT))
    clock = p.time.Clock()
    screen.fill(p.Color("white"))
    gs = GameState()

    loadImages()    #Tylko raz ładujemy
    running = True  #Rozgrywka on
    sqSelected = () #Żadne kwadraty nie są wybrane
    from_to = []    #Trzymamy ruchy gracza (dwa słowniki: [(6, 4), (4,4,)])


    # ----------------------------------------------------------------------------------------------------------------------------------
    

    #Pierwsze narysowanie planszy na podstawie planszy zainicjowanej w __init__
    rysujPlansze(screen, gs, kom, nick, komunikat_ruch, clock)
    

    # ROZGRYWKA
    while running:

        #Dopóki nie dostaniemy komunikatu, że gracz czarny się połączył
        if komunikat == "Grasz białymi - Czekaj na przystąpienie partnera":

            inputPO = socket1.recv(2000)
            if kolor_gracza == "white":
                komunikat = konwertujInput(inputPO)[0]
            else:
                komunikat = konwertujInput(inputPO)[1]
            tab_akt = konwertujInput(inputPO)[3]
            kogo_ruch = konwertujInput(inputPO)[4]

            licznik += 1
            print(licznik, "\nOdebrałem czekając na gracza\n")
            for i in range(8):
                print(tab_akt[i])
            print("\nKomunikat: ", komunikat)
            print("Czyj ruch: ", kogo_ruch, "\n")



        while kolor_gracza != kogo_ruch:
            licznik += 1
            print(licznik, "\nODEBRAŁEM W WHILE")
            inputPO = socket1.recv(2000)
            if kolor_gracza == "white":
                komunikat = konwertujInput(inputPO)[0]
            else:
                komunikat = konwertujInput(inputPO)[1]
            tab_akt = konwertujInput(inputPO)[3]
            kogo_ruch = konwertujInput(inputPO)[4]
            for i in range(8):
                print(tab_akt[i])
            print("\nKomunikat: ", komunikat)
            print("Czyj ruch: ", kogo_ruch, "\n")


        move = Move((0,0), (0, 0), tab_konw)
        gs.makeMove(move, tab_akt)

        kom = myfont.render(komunikat, True, (0, 0, 0))
        komunikat_ruch = myfont.render(''.join(["Teraz ruch ma ", kogo_ruch]), True, (0, 0, 0))

        #Odswiezamy tablice
        rysujPlansze(screen, gs, kom, nick, komunikat_ruch, clock)


        # jeden ruch
        for e in p.event.get():
            #Jak wciśniemy X to koniec gry
            if e.type == p.QUIT:
                running = False
            #Wykrywamy ruch myszy
            elif e.type == p.MOUSEBUTTONDOWN:
                location = p.mouse.get_pos()  # (x,y) lokalizacja myszy
                col = location[0] // SQ_SIZE
                row = location[1] // SQ_SIZE
                #Gracz kliknął ten sam kwadrat dwa razy, więc resetujemy
                if sqSelected == (row, col):
                    sqSelected = ()  # odznacz
                    from_to = []     # wyczysc
                else:
                    sqSelected = (row, col)
                    from_to.append(sqSelected)  # dodaj 1 i 2 klikniecie



#--------------- GRACZ WYKONAL RUCH --------------------------------------------------------------------
                if len(from_to) == 2:  # po 2 kliknięciach - zrobieniu ruchu

                    # WYSYŁAM SERVEROWI    
                    wiadomosc = konwertujOutput(from_to, kolor_gracza)
                    do_wyslania = wiadomosc.encode('utf-8')
                    socket1.send(do_wyslania) 


                    #ODBIERAM OD SERVERA
                    inputPO = socket1.recv(2000)
                    tab_akt = konwertujInput(inputPO)[3]
                    kogo_ruch = konwertujInput(inputPO)[4]                    
                    if kolor_gracza == "white":
                        komunikat = konwertujInput(inputPO)[0]
                    else:
                        komunikat = konwertujInput(inputPO)[1]
                    kom = myfont.render(komunikat, True, (0, 0, 0))
                    komunikat_ruch = myfont.render(''.join(["Teraz ruch ma ", kogo_ruch]), True, (0, 0, 0))

                    licznik += 1
                    print(licznik, "\nODEBRAŁEM W IF:")
                    for i in range(8):
                        print(tab_akt[i])
                    print("\nKomunikat: ", komunikat)
                    print("Czyj ruch: ", kogo_ruch, "\n")


                    move = Move(from_to[0], from_to[1], tab_konw)
                    gs.makeMove(move, tab_akt)

                    sqSelected = ()  # zresetuj nasze kliknięcia
                    from_to = []

#---------------------------------------------------------------------------------------------------------
        #Dopiero tu rysujemy planszę
        rysujPlansze(screen, gs, kom, nick, komunikat_ruch, clock)

    #ZAMYKANIE DESKRYPTORA
    socket1.close()
# ------------------------------------------------------------------------------------------------------




if __name__ == "__main__":
    main()
