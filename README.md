# Radar Simulator

Simulatore didattico di un sistema radar scritto in C e Java.

Il progetto genera echi di bersagli e rumore, applica un rilevamento a soglia, trasmette i contatti rilevati tramite un server TCP in C (socket POSIX) e li visualizza in un client grafico Java su uno schermo radar.

È un progetto personale nato per consolidare C su Linux, programmazione di rete con socket, build automatizzata, test e integrazione di componenti scritti in linguaggi diversi.

## Indice

1. [Obiettivi](#obiettivi)
2. [Architettura](#architettura)
3. [Modello del segnale](#modello-del-segnale)
4. [Protocollo di comunicazione](#protocollo-di-comunicazione)
5. [Struttura del repository](#struttura-del-repository)
6. [Requisiti](#requisiti)
7. [Build ed esecuzione](#build-ed-esecuzione)
8. [Test](#test)
9. [Risultati](#risultati)
10. [Scelte progettuali](#scelte-progettuali)
11. [Limiti noti](#limiti-noti)
12. [Sviluppi futuri](#sviluppi-futuri)

## Obiettivi

- Scrivere software in C su Linux con attenzione a warning del compilatore e gestione degli errori.
- Usare l'API dei socket (`socket`, `bind`, `listen`, `accept`, `write`) per realizzare un server TCP.
- Definire un protocollo testuale semplice tra due componenti scritti in linguaggi diversi.
- Automatizzare build e test con `make` e uno script di test.
- Verificare il funzionamento su Linux, anche lavorando da macOS.
- Capire il compromesso tra probabilità di rilevazione e falsi allarmi in un rilevatore a soglia.

## Architettura

```
Generatore echi + rumore  ->  Rilevamento a soglia  ->  Server TCP  ->  Client Java (GUI)
          (C)                        (C)                  (C)             (Swing)
```

| Componente | File | Ruolo |
|---|---|---|
| Simulatore | `radar_sim.c` | Genera 10 bersagli e 30 contatti di solo rumore, applica la soglia e stampa bersagli rilevati, mancati e falsi allarmi |
| Server | `radar_server.c` | Genera una scansione al secondo e invia ai client i soli contatti sopra soglia |
| Client | `RadarClient.java` | Si collega al server, interpreta le scansioni e le disegna su uno schermo radar |

## Modello del segnale

Ogni scansione contiene:

- **10 bersagli** con distanza casuale tra 1 e 100 km e azimut tra 0° e 359°.
- **30 contatti di solo rumore**, con distanza e azimut casuali.

L'ampiezza dell'eco di un bersaglio segue la relazione semplificata dell'equazione radar, in cui la potenza ricevuta diminuisce con la quarta potenza della distanza:

```
ampiezza = K / R^4 + rumore
```

dove `K = 1e8`, `R` è la distanza in km e il rumore è uniforme nell'intervallo `[0, 6)`. I contatti di solo rumore hanno ampiezza uguale al solo rumore.

Un contatto è considerato **rilevato** se la sua ampiezza supera la soglia.

## Protocollo di comunicazione

Il server invia testo, una riga per messaggio. Ogni scansione ha questa forma:

```
SCAN <numero>
<id>,<distanza_km>,<azimut_gradi>,<ampiezza>
<id>,<distanza_km>,<azimut_gradi>,<ampiezza>
...
END
```

- L'azimut è misurato da 0° (nord) in senso orario.
- Il server non indica se un contatto sia un bersaglio reale o un falso allarme: come in un radar vero, è il client (o l'operatore) a dover interpretare i contatti.
- Una nuova scansione viene inviata ogni secondo.

## Struttura del repository

```
radar-simulator/
├── radar_sim.c        simulatore con rilevamento a soglia
├── radar_server.c     server TCP
├── RadarClient.java   client grafico
├── Makefile           build e test
├── test.sh            test automatico del server
├── .gitignore
└── README.md
```

## Requisiti

- `gcc` (o `clang`) e `make`
- Java 17 o superiore (il client usa i `record`)
- Linux o macOS
- Opzionale: Docker, per provare il progetto in un ambiente Linux pulito

## Build ed esecuzione

Compilazione:

```
make
```

Simulatore a riga di comando (stampa i contatti rilevati e le statistiche):

```
./radar_sim
```

Server e client:

```
./radar_server
```

In un secondo terminale, nella stessa cartella:

```
javac RadarClient.java
java RadarClient
```

Si apre una finestra con cerchi a 25, 50, 75 e 100 km. A ogni scansione compaiono i contatti sopra soglia. Le ultime 5 scansioni restano visibili con intensità decrescente.

Il server usa la porta **5050**. Su macOS la porta 5000 è occupata da AirPlay Receiver, per questo non è stata usata. Per fermare il server premere `Ctrl+C`.

Pulizia dei file generati:

```
make clean
```

## Test

```
make test
```

Lo script `test.sh` avvia il server, si collega alla porta 5050 e controlla che arrivino i messaggi `SCAN 1` ed `END`. Stampa `TEST OK` oppure `TEST FALLITO`.

Per eseguire build e test in un ambiente Linux pulito, da una cartella con Docker attivo:

```
docker run --rm -v "$(pwd)":/work -w /work gcc:latest bash -c "make clean && make && make test"
```

Il progetto compila senza warning con `-Wall -Wextra -O2` ed è stato testato in questo modo su Linux. Dopo il test in Docker, su macOS eseguire `make clean && make`, perché i binari creati nel container sono per Linux.

## Risultati

Esempi da singole esecuzioni di `radar_sim`. I valori cambiano a ogni esecuzione perché i dati sono casuali.

| Soglia | Bersagli rilevati | Falsi allarmi |
|--------|-------------------|---------------|
| 2.0    | 10/10             | 20            |
| 8.0    | 7/10              | 0             |
| 10.0   | 4/10              | 0             |

- Con soglia bassa vengono rilevati quasi tutti i bersagli, ma molti contatti di solo rumore la superano e producono falsi allarmi.
- Con soglia alta i falsi allarmi scompaiono, ma si perdono i bersagli lontani, che hanno un eco più debole.
- Nel server è impostata la soglia 8.0.

## Scelte progettuali

- **Protocollo testuale**: è semplice da leggere e da verificare con strumenti come `nc`, e rende il client indipendente dal linguaggio del server.
- **Server che non etichetta i contatti**: rispecchia la situazione reale, in cui bersagli e falsi allarmi non sono distinguibili dalla sola ampiezza.
- **Thread separato per la rete nel client**: la lettura dal socket avviene in un thread dedicato e gli aggiornamenti grafici passano da `SwingUtilities.invokeLater`, per non bloccare l'interfaccia.
- **Gestione del client disconnesso**: nel server `SIGPIPE` è ignorato e gli errori di `write` fanno chiudere la connessione, senza terminare il processo. Il server torna in attesa di un nuovo client.
- **`SO_REUSEADDR`** sul socket del server, per poterlo riavviare subito senza attendere il rilascio della porta.

## Limiti noti

- La soglia è fissa. I radar reali usano soglie adattive (per esempio CFAR).
- Il rumore è uniforme e limitato a `[0, 6)`. Con una soglia superiore a 6 non si possono quindi avere falsi allarmi, mentre nei sistemi reali il rumore è illimitato (tipicamente gaussiano), e un falso allarme resta sempre possibile.
- I dati sono simulati e non derivano da un segnale reale, e non c'è elaborazione Doppler.
- Il server gestisce un solo client alla volta.
- Indirizzo e porta del client sono scritti nel codice (`localhost:5050`).
- Il test controlla la presenza dei messaggi `SCAN 1` ed `END`, non il contenuto dei contatti.

## Sviluppi futuri

- Soglia configurabile da riga di comando.
- Rumore gaussiano e soglia adattiva.
- Server con più client in parallelo.
- Tracciamento dei bersagli tra scansioni successive.
- Test più completi sul formato dei messaggi.