import sys  # Necesari per  sys.exit()
import argparse
import os

from pathlib import Path
from pysat.formula import CNF # type: ignore #Cal instalar pip install python-sat

num_files=0
num_columnes=0
info_files=[]
info_columnes=[]



tauler=[]
variable=1 #El 1 no s'utilitzara. Important
getTrue = 1 #//Trivially true variable. Used as the "true" constant
getFalse = -1 #//Trivially false variable. Used as the "false" constant


modeSelect=2
fer_ENC3=False


def new_variable():
    global variable
    variable=variable+1
    return variable

def newVarArray(k:int):
    return  [new_variable() for _ in range(0,k)]



def processar_info(info):
    """
    Valida i procesa una llista:
    - LLenca una excepcio si hi ha numeros negatius.
    - Si la longitud es major a 1, elimina els 0.
    - Si la llista queda buida despres d'eliminar 0, retorna [0].
    """

    # Si la longitud es 1, retornar la llista tal como està (permet [0] i [<0])
    if len(info) == 1:
        return info

    #if any(x < 0 for x in info):
    #    raise ValueError("No numeros negatius")

    # Si la longitud es superior a 1, eliminem els 0 o valors negatius. 
    info = [x for x in info if x > 0]

    # Si la llista queda buida, retornem la llista [0]
    if not info:
        return [0]

    return info

def llegir_info_nonograma_arxiu(nomArxiu):
    global num_files, num_columnes, info_files, info_columnes

    try:
        with open(nomArxiu, 'r') as arxiu:
            # Llegir les dimensiones
            linia = list(map(int, arxiu.readline().strip().split()))
            if len(linia) != 2:
                raise ValueError("El rxiu no te les dimensions valides.")
            
            num_files, num_columnes = linia[0], linia[1]
            # Llegir les restricciones de les filas
            info_files = []
            for _ in range(num_files): 
                linia = arxiu.readline().strip() #strip elimina posibles \n o \r que puguin haver
                info = [int(x) for x in linia.split()]  # Transformar a enters
                info = processar_info(info)  #Verifiquem la informacio
                info_files.append(info)

            # Llegir les restricciones de les columnas
            info_columnes = []
            for _ in range(num_columnes):
                linia = arxiu.readline().strip() #strip elimina posibles \n o \r que puguin haver
                info = [int(x) for x in linia.split()]  # Transformar a enters
                info = processar_info(info)  #Verifiquem la informacio
                info_columnes.append(info)

    except FileNotFoundError:
        raise FileNotFoundError(f"No s'ha pogut obrir el arxiu '{nomArxiu}'. Verifica que l'arxiu existeix o que el nom sigui correcte.")
    except ValueError as e:
        raise ValueError(f"Error al procesar l'arxiu: {e}")

"""Per introduir manualment el nonograma"""
def llegir_info_nonograma():

    global num_files, num_columnes, info_files, info_columnes

    linia = input() #Llegeix tota la linia

    num_files, num_columnes = [int(x) for x in linia.split()] #Transformara cada valor a int de lina.split()

    if num_files<=0 | num_columnes<=0 :
        raise ValueError("Dades nombre de files i columnes no positives mes gran que 0")
    
    print(f"Files: {num_files}")
    print(f"Columnes: {num_columnes}")

    for _ in range(num_files):
        linia = input()
        info=[int(x) for x in linia.split()] #Transformara cada valor a int de lina.split()
        
        info=processar_info(info)
        info_files.append(info)
    
    for _ in range(num_columnes):
        linia = input()
        info=[int(x) for x in linia.split()]
        
        info=processar_info(info)
        info_columnes.append(info)

    #Per observar els resultats
    # print("Files:")
    # for i in info_files:
    #     print(i)
    # print("Columnes:")
    # for i in info_columnes:
    #     print(i)


def asignar_variables_tauler():
    global tauler,num_files,num_columnes
    tauler = [newVarArray(num_columnes) for _ in range(num_files)]
              

"""Funcio per escriure el Tauler"""
def generar_for_output(tauler,nfiles,nColumnes,nomarxiuResultant):

    with open(nomarxiuResultant, 'w') as archivo:
        # Less dimensiones
        archivo.write(f"{nfiles} {nColumnes}\n")
        
        # Escriu els valors de la matriu (tauler)
        for fila in tauler:
            archivo.write(" ".join(map(str, fila)) + "\n")


"""Modelitzacio per EK. Extret del treball de ScalAT de l'assignatura Programació declarativa. Aplicacions"""
def addCMP2(x1: int, x2: int, y1: int, y2: int) -> list[list[int]]:
    llistaClausules=[]
    #y1 <-> x1 \/ x2

    llistaClausules.append([-y1,x1,x2])#y1 -> x1 \/ x2

    llistaClausules.append([-x1,y1]) #y1 <- x1 \/ x2
    llistaClausules.append([-x2,y1])

    #y2 <-> x1 /\ x2

    llistaClausules.append([-y2,x1])#y2 -> x1 /\ x2
    llistaClausules.append([-y2,x2])

    llistaClausules.append([-x1,-x2,y2])#y2 <- x1 /\ x2

    return llistaClausules

def isPowerOfTwo(x: int) -> bool: 
    n = x
    if (n == 0):
        return False
    while (n != 1):
      if (n % 2 != 0):
        return False
      n = n / 2
    return True

def addMerge(x: list[int], xp: list[int], y: list[int])-> list[list[int]] :
    """ Adds the encoding of 'merge the decreasingly sorted lists x and xp into y'
        x and xp must be of equal lenght and nonempty. y must have twice the lenght of x
        The lists contain literals
        All variables must have been created with one of the newVar methods.
    """
    llistaClausules=[]
    assert len(x)==len(xp), "les llistes X i Y han de ser d'igual mida"
    assert len(x)>0, "La llista X no pot estar buida"
    assert 2*len(x) == len(y)

    if len(x)== 1:
        novesClausules=addCMP2(x[0],xp[0],y[0],y[1])
        for c in novesClausules:
            llistaClausules.append(c)
    else:
        # Dividir `x` en pars e impars
        xeven = [x[i] for i in range(0, len(x) - 1, 2)] #Agafa els valors de x en les posicions pars. De [0,len(x)-1) saltant de dos en dos
        xodd = [x[i] for i in range(1, len(x), 2)]

        # Dividir `xp` en pars e impars
        xpeven = [xp[i] for i in range(0, len(x) - 1, 2)]
        xpodd = [xp[i] for i in range(1, len(x), 2)]

        # Crear nuoves variables intermitges
        zevenp = newVarArray(len(x) - 1)
        zoddp = newVarArray(len(x) - 1)
        zeven = [y[0]] + zevenp
        zodd = zoddp + [y[-1]]

        #Crides Recursives
        llistamerge0=addMerge(xeven,xpeven,zeven)
        llistamerge1=addMerge(xodd,xpodd,zodd)

        for c in llistamerge0:
            llistaClausules.append(c)
        for c in llistamerge1:
            llistaClausules.append(c)

        # Comparacions entre elements de `zevenp` y `zoddp`
        for k, ((i, j)) in enumerate(zip(zevenp, zoddp), start=1):
            clausuleComparacions=addCMP2(i, j, y[2 * k - 1], y[2 * k])
            for c in clausuleComparacions:
                llistaClausules.append(c)
    return llistaClausules

def addSorter(x: list[int], y: list[int]) -> list[list[int]]:
    llistaClausules=[]
    assert len(x)==len(y), "les llistes X i Y han de ser d'igual mida"
    assert len(x)>0, "La llista X no pot estar buida"

    if(len(x)==1):
        llistaClausules.append([-x[0],y[0]]) #x[0] <-> y[0]
        llistaClausules.append([x[0],-y[0]])
    elif (len(x)==2):
        novesClausules=addCMP2(x[0],x[1],y[0],y[1])
        for c in novesClausules:
            llistaClausules.append(c)
    else:
        xp = []
        yp = []
        while not isPowerOfTwo(len(x)+len(xp)):
            xp.append(getFalse)
            yp.append(getFalse)
        
        xp = x + xp #Concatenar
        yp = y + yp


        # División de la llista xp en dos meitats
        x1 = xp[:len(xp) // 2] #Primera meitat
        x2 = xp[len(xp) // 2:] #Segona meitat

        z1 = newVarArray(len(xp) // 2 )
        z2 = newVarArray(len(xp) // 2 )

        llistaSorter1 = addSorter(x1,z1)
        llistaSorter2 = addSorter(x2,z2)
        llistaMerge = addMerge(z1,z2,yp)

        for c in llistaSorter1:
            llistaClausules.append(c)
        for c in llistaSorter2:
            llistaClausules.append(c)
        for c in llistaMerge:
            llistaClausules.append(c)

    return llistaClausules

def EK(x: list[int], k: int) -> list[list[int]]:
    """Pre: Tots els valors de x son enters positius. Son variables"""
    llistaClausules: list[list[int]] = []
    if k<=0: #Vol que 0 o menys estiguin a true. Vol dir tot a fals
        for v in x:
            llistaClausules.append([-v]) #Unitaries tot a fals
    elif k>=len(x): #Vol que k o mes estiguin a true. Vol dir tot a true
        for v in x:
            llistaClausules.append([v]) #Unitaries tot a true
    else:
        y=newVarArray(len(x))
        clausulesSort=addSorter(x,y)
        for c in clausulesSort:
            llistaClausules.append(c)
        
        llistaClausules.append([y[k-1]])
        llistaClausules.append([-y[k]])
    
    return llistaClausules


"""generar arxiu DIMACS"""
def toFile(clausules, nomArxiu):
    cnf = CNF()
    
    for clausula in clausules:
        cnf.append(clausula)

    cnf.to_file(nomArxiu)


def encodingFilaEficient(x: list[int],info: list[int]) -> list[list[int]]:

    if len(info)==1:
        if info[0]<0:#Amb valors <0 (normalment posar -1) indiquem que no volem codificar aquesta fila, per deixarla ambigua
            return []
        
        if info[0]==0:#Amb valor 0, indiquem que es una fila buida
            return [ [-v] for v in x ]

        if info[0]==0:#indiquem que es una fila es plena
            return [ [v] for v in x ] 
        

    assert sum(info)+len(info)-1 <= len(x), "No hi ha prou espai per posar blocs a aquesta fila " + str(info) 

    llistaClausules=[]

    #Variables auxiliars inici de bloc
    y = [newVarArray(len(x)) for _ in range(len(info))]

    varEnriquit=[]
    clausules_per_enriquir=[]

    if fer_ENC3:

        #Variables Presencia de bloc
        z=[newVarArray(len(x)) for _ in range(len(info))]#Variables per enriquir
        
        #x[i][j] es variables[j]
        #z[i][j][k] es varEnriquit[k][j]
        #y[i][j][k] es hasStarted[k][j]
        """
        z[i][j][k]: la pista k de la fila i emplena la columna j

        zf[i][j][k] <-> yf[i][j][k] and -yf[i][j-valorPista(k)][k]

        x[i][j] <-> zf[i][j][1] or zf[i][j][2] or ... or zf[i][j][k]
        
        """

        """Implementacio per zf[i][j][k] <-> yf[i][j][k] and -yf[i][j-valorPista(k)][k]"""
        for k in range(0,len(info)):#Per cada pista (block)
            for j in range(0,len(x)):
                varZ=z[k][j]
                varYf1=y[k][j]
                varYf2=y[k][j-info[k]]

                """zf[i][j][k] -> yf[i][j][k] and -yf[i][j-valorPista(k)][k]
                 equival:
                 (-zf[i][j][k] or yf[i][j][k]) and ( -zf[i][j][k] or -yf[i][j-valorPista(k)][k])
                """
                llistaClausules.append([-varZ,varYf1])
                if(j-info[k]>=0): #Cal tenir en compte que si surts de rang per l'esquerra, el literal esperat es com si no estigues (nomes tens la binaria amb Yf1). Python amb negatius el que fa es accedir a la taula per el final
                    llistaClausules.append([-varZ,-varYf2])
                
                """zf[i][j][k] <- yf[i][j][k] and -yf[i][j-valorPista(k)][k]"""
                if(j-info[k]>=0):
                    llistaClausules.append([varZ,-varYf1,varYf2])
                else:
                    llistaClausules.append([varZ,-varYf1])

                    
                
        
        """Implementacio per x[i][j] <-> zf[i][j][1] or zf[i][j][2] or ... or zf[i][j][k]"""
        for j in range(0,len(x)):
            varX=x[j]
            variablesZ=[]
            for k in range(0,len(info)):
                varZact=z[k][j]
                variablesZ.append(varZact) #Ens apuntem la variable Z, que necesitem crear la clausula amb totes

                """x[i][j] <- zf[i][j][1] or zf[i][j][2] or ... or zf[i][j][k] 
                (x[i][j] or -zf[i][j][1]) and (x[i][j] or -zf[i][j][2]) and ... and (x[i][j] or -zf[i][j][k]
                """
                llistaClausules.append([varX,-varZact])#Aixi anirem afegint les binaries corresponents

            #variablesZ te totes les varZ necesaries(en positiu)
            variablesZ.append(-varX) #Ara es la clausula que implementa x[i][j] -> zf[i][j][1] or zf[i][j][2] or ... or zf[i][j][k]
            
            llistaClausules.append(variablesZ)

    
    
    """
     If a block of a row has started at column i, it also must have started in column i+1.
        //Order encoding
        forall(i in 0..rowSize-1, b in 0..maxNonos-1){
            if(rowNonos[i][b] != 0){
                forall(j in 0..colSize-2){
                    hasStartedRow[i][b][j] -> hasStartedRow[i][b][j+1];
                };
            }
            else{
                &&( [!hasStartedRow[i][b][j] | j in 0..colSize-1] );
            };
         };

    """
    clausulesHasStartedBlock=[]
    for b in range(0,len(info)):
        for j in range (0,len(x) -2 +1):
            # (y[b][j] -> y[b][j+1])
            llistaClausules.append([-y[b][j],y[b][j+1]])
    

    """A block mush have started soon enough to fit in the row.
        forall(i in 0..rowSize-1, b in 0..maxNonos-1){
            if(rowNonos[i][b] != 0){
                hasStartedRow[i][b][colSize-rowNonos[i][b]];
            };
        };
    """
    for b in range (0,len(info)):
        llistaClausules.append([y[b][len(x)-info[b]]])

    
    """x[i][j] must be true if it is colored.
        //Channelling between hasStarted and x
        forall(i in 0..rowSize-1, b in 0..maxNonos-1){
            if(rowNonos[i][b] != 0){
                forall(j in 0..colSize-1){
                    if(j >= rowNonos[i][b]){
                        x[i][j] <- hasStartedRow[i][b][j] & !hasStartedRow[i][b][j-rowNonos[i][b]];
                    }
                    else {
                        x[i][j] <- hasStartedRow[i][b][j];
                    };
                };
            };
         }; 
    """    
    
    if not fer_ENC3:#Si es fa el mode 3, aquest no cal
        for b in range(len(info)):
            for j in range(0,len(x)):
                if(j>=info[b]):
                    llistaClausules.append([x[j],-y[b][j],y[b][j-info[b]]]) #x[i][j] <- hasStartedRow[i][b][j] & !hasStartedRow[i][b][j-rowNonos[i][b]];
                else:
                    llistaClausules.append([x[j],-y[b][j]]) #x[i][j] <- hasStartedRow[i][b][j]


    """
    The number of cells true in the row i must be the sum of the length of the blocks in row i
    forall(i in 0..rowSize-1){
        EK(x[i], sum(rowNonos[i]));
    }; 
    """
    clausulesEK=[]
    if not fer_ENC3 or not versio2:
        clausulesEK=EK(x,sum(info))
        llistaClausules.extend(clausulesEK) 

    """Block b must start before block b+1
        forall(i in 0..rowSize-1, b in 0..maxNonos-2){
            if(rowNonos[i][b+1] != 0){
                forall(j in 0..colSize-1){
                    if(j-rowNonos[i][b]-1 >= 0){
                        hasStartedRow[i][b+1][j] -> hasStartedRow[i][b][j-rowNonos[i][b]-1];
                    }
                    else {
                        !hasStartedRow[i][b+1][j];
                    };
                };
            };
        }; """
    for b in range(0,len(info)-1):
        for j in range(0,len(x)):
            if j-info[b]-1>=0:
                llistaClausules.append([-y[b+1][j],y[b][j - info[b]-1]])
            else:
                llistaClausules.append([-y[b+1][j]])

    
    return llistaClausules

# def codificacioEficientNonograma() -> list[list[int]]:
#     global tauler,num_files,num_columnes,info_files,info_columnes

#     llistaClausules=[]

#     tauler = [ newVarArray(num_columnes) for _ in range(0,num_files)] #Generem el tauler

#     for f in range(0,num_files):
#         # print(info_files[f])
#         clausulesObtingudes=encodingFilaEficient(tauler[f],info_files[f])
#         for c in clausulesObtingudes:
#             llistaClausules.append(c)
        

#     #Obtenir la transposada
#     taulerTransposada = list(zip(*tauler))
#     taulerTransposada = [list(fila) for fila in taulerTransposada]

#     for c in range(0,num_columnes):
#         # print(info_columnes[c])
#         clausulesObtingudes=encodingFilaEficient(taulerTransposada[c],info_columnes[c])
#         for c in clausulesObtingudes:
#             llistaClausules.append(c)

#     return llistaClausules


def generarCombinacions(info:list[int],mida: int) -> list[list[int]]:
            
    llistaCombinacions=[]
    assignacio01=[0]*mida
    
    def generacioCombinacionsR(posInicial,blocActual,assignacio):
        midaBloc=info[blocActual]
        if blocActual == len(info)-1:
            for j in range(posInicial, mida - midaBloc + 1):
                for i in range(midaBloc):
                    assignacio[j+i]=1
                llistaCombinacions.append(assignacio.copy())
                for i in range(midaBloc):
                    assignacio[j+i]=0
        else:
            blocsPosteriors=info[blocActual+1:]
            posFinalATrue=mida-sum(blocsPosteriors)-len(blocsPosteriors)-midaBloc
            for j in range(posInicial, posFinalATrue + 1):
                for i in range(midaBloc):
                    assignacio[j+i]=1
                generacioCombinacionsR(j+midaBloc+1,blocActual+1,assignacio)
                for i in range(midaBloc):
                    assignacio[j+i]=0
        
    generacioCombinacionsR(0,0,assignacio01)
    return llistaCombinacions


def encodingENC1NoEficient(x:list[int],info:list[int]) -> list[list[int]]:

    if len(info)==1:
        if info[0]<0: #Indicacio que no es vol codificar la fila
            return []
        if info[0]==0: #Indicacio que la fila es buida
            return [[-v] for v in x]
        if info[0]==len(x): #Indicacio que la fila es plena
            return [[v] for v in x]
            

    llistaClausules=[]
    combinacions=generarCombinacions(info,len(x))
    llista_auxiliars=[]
    if len(combinacions)==1:
         assignacio01=combinacions[0]
         llistaClausules.extend( [ [v] if b == 1 else [-v] for v, b in zip(x, assignacio01)] )
    else:
         llista_auxiliars=[]
         for c in range(len(combinacions)):
            p=new_variable()
            llista_auxiliars.append(p)
            assignacio01=combinacions[c]
            assignacio=[ v if b == 1 else -v for v, b in zip(x,assignacio01)]
         
            """ (p -> assignacio)"""
            for v in assignacio:
                llistaClausules.append([-p,v])

            """ (p <- assignacio)"""
            llistaClausules.append([p] + [-v for v in assignacio])

    """(p_1 or p_2 or ... or p_n)"""
    llistaClausules.append(llista_auxiliars)

    return llistaClausules

def obtenirClausulesNonograma():
    global tauler, num_files, num_columnes, info_files, info_columnes
    llistaClausules = []

    for f in range(num_files):
        if modeSelect == 1:
            restriccions = encodingENC1NoEficient(tauler[f], info_files[f])
        else:
            restriccions = encodingFilaEficient(tauler[f], info_files[f])
        llistaClausules.extend(restriccions)

    taulerTransposada = list(zip(*tauler))
    taulerTransposada = [list(fila) for fila in taulerTransposada]

    for c in range(num_columnes):
        if modeSelect == 1:
            restriccions = encodingENC1NoEficient(taulerTransposada[c],info_columnes[c])
        else:
            restriccions = encodingFilaEficient(taulerTransposada[c],info_columnes[c])
        llistaClausules.extend(restriccions)

    return llistaClausules
        

def def_main():
    # Crear un analitzador d'arguments 
    parser = argparse.ArgumentParser(description="Un programa que crea una codificacio SAT d'un nonograma.")
    
    parser.add_argument('-i', '--input', type=str, default=None, help="Ruta del arxiu de la info del nonograma.")

    parser.add_argument('-m','--mode',type=int,default=2, help="Mode: avisem si volem la codificacio eficient[2], eficient i enriquit[3] o no eficient[1]")

    parser.add_argument('-v2', '--versio2', action='store_true', help="Activa la versió dos del mode 3 (sense el EK)")

    

    args = parser.parse_args()
    

    return args.input, args.mode, args.versio2

def obtenirNom(pathArxiu: str) -> str:
    
    nom = Path(pathArxiu).stem #Treu el path i l'extensio. Nomes es queda amb el nom del arxiu
    return nom


# Bloc main
if __name__ == "__main__":
    pathnomArxiu  , modeSelect , versio2= def_main()

    nomArxiu=""
    
    if modeSelect==1:
        codificacioEficient=False
    else:
        codificacioEficient=True
        if modeSelect==3:
            fer_ENC3=True

    try:
        if pathnomArxiu is None:
            llegir_info_nonograma()
            nomArxiu="NONOGRAMA_BASIC"
        else:
            llegir_info_nonograma_arxiu(pathnomArxiu)
            nomArxiu=obtenirNom(pathnomArxiu)
    except ValueError as e:
        print(f"S'ha produit un error: {e}")
        """El nonograma no es valid"""
        sys.exit(1)

    

    codificaciSAT_clausules=[]
    asignar_variables_tauler()#Generem Tauler
    codificaciSAT_clausules=obtenirClausulesNonograma()

        

    nomEncoding=""
    if codificacioEficient:
        if fer_ENC3:
            nomEncoding="ENC3"
        else:
            nomEncoding="ENC2"
    else:
        nomEncoding="ENC1"
    
    carpeta = "Encodings/ENC2_Eficient/" if codificacioEficient else "Encodings/ENC1_NoEficient/"
    carpeta = "Encodings/ENC3_EficientEnriquit/" if fer_ENC3 else carpeta
    if fer_ENC3:
        if versio2:
            carpeta = "Encodings/ENC3_EficientEnriquitV2/"
    carpeta_taulers = os.path.join(carpeta, "Taulers")

    # Crear las carpetas si no existeixen
    os.makedirs(carpeta, exist_ok=True)
    os.makedirs(carpeta_taulers, exist_ok=True)

    # Guardar els arxius
    toFile(codificaciSAT_clausules, os.path.join(carpeta, nomEncoding + "_" + nomArxiu + ".dimacs"))
    generar_for_output(tauler, num_files, num_columnes, os.path.join(carpeta_taulers, "tauler_" + nomEncoding + "_" + nomArxiu + ".txt"))

    

    
    

