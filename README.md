Per effetuare i test è sufficente l'esecuzione di:
    test.sh

Il test effettua un numero di operazioni pari a 1000, 10000, 1000000, 1000000 con rapporto scritture/letture da 95%/5% a 30%/70%. Il test verrà ripetuto 128 volte per ottenere una media del tempo di esecuzione. Questa verrà fatto effetuando, o meno, una pulizia della cache prima di ogni ripetizione.
Nel caso di MVM_CKPT il test viene ripetuto per le tre modalità di checkpointing (64,128,256) e per tre tagli di memmoria 1MB, 2MB, 4MB.
In MVM e MVM_CKPT è stato inibito il sistema di patch per le operazioni di load.
Inoltre sono state rimosse tutte le operazione di AUDIT.

Lo script effettuerà le segeunti operazioni:
- esecuzione di MVM, con la patch presente in https://github.com/FrancescoQuaglia/MVM;
- esecuzione di MVM_CKPT;
- creazione di un virtual environment di Python;
- attivazione del venv;
- installazione dei requisiti necessari a generari i grafici;
- esecuzioni di graphs.py con generazioni dei grafici nelle directory graphs_mvm e graphs_ckpt.