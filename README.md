Per effetuare i test è sufficente l'esecuzione di:
    sh test.sh

Il test effettua un numero di operazioni pari a 1000, 10000, 1000000, 1000000 con rapporto scritture/letture da 95%/5% a 30%/70%. 
Il test verrà ripetuto 50 volte per ottenere una media del tempo di esecuzione eun intervallo di confidenza del 95%. 
Il test verrà eseguito effetuando, o meno, un'operazione di cache flush su tutta la memoria utilizzata prima delle scritture/letture e prima delle operazioni di restore.
Nel caso di MVM_GRID_CKPT_BS il test viene ripetuto per le quattro modalità di checkpointing (8,16,32,64 byte) e per tre tagli di memmoria 1MB, 2MB, 4MB.
Stessa cosa per MVM che ripete le stesse operazione, ma si basa su una patch C.
SIMPLE_CKPT, effettua una memcpy dell'area di memoria prima delle scritture/letture e una memcpy come operazione di restore.

Lo script effettuerà le segeunti operazioni:
- esecuzione dei test per MVM_GRID_CKPT_BS (https://github.com/mazzucchi-andrea/MVM/tree/GRID_CKPT);
- esecuzione dei test per MVM, con pacth C (https://github.com/mazzucchi-andrea/MVM/tree/GRID_CKPT_C_Patch);
- esecuzione dei test per SIMPLE_CKPT, checkpoint che utilizza memcpy;
- creazioni dei grafici tramite gnuplot.