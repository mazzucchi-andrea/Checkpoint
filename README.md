# TestCheckpoint

Consideriamo come riferimento un’area di memoria contigua di 8256 byte, quest’area è divisa in 3 sezioni:

- **A**, l’area di lavoro, di 4096 byte
- **S**, l’area di snapshot di 4096 byte
- **M**, l’area dei meta-dati di 64 byte.

L’indirizzo di destinazione dell’instrumentazione della memoria viene considerato presente in `rax` e denominato `addressA`.

Proseguiamo con il commento dell’algoritmo.

                mov %rax, %gs:0
                mov %rbx, %gs:8
                mov %rcx, %gs:16

I registri `rax`, `rbx` e `rcx` verranno utilizzati in seguito quindi prima di altre operazioni si procede al loro salvataggio in **TLS** agli offset `0`, `8` e `16`.

                mov %rax, %rcx
                and $-4096, %rcx

`rcx` verrà utilizzato per contenere nel corso di tutta l’esecuzione l’indirizzo base dell’area di memoria. L’operazione di and permette di rimuovere gli ultimi 12 bit che contengono l’offset.

				and $0xFFF, %rax

`rax` a seguito di questa operazione di and conterrà l’offset su cui si opererà per determinare il bit nella bitmap nei meta-dati che individua la quadword, o le quadword nel caso di accesso non allienato.

				test $7, %rax
				jz second_qword

Nel caso l’offset sia allineato è sufficiente operare solo sulla quadword puntata da addressA, altrimenti sulle 2 quadword allineate che la contengono.

				and $-8, %rax

Effettuando l’operazione di and bit a bit con -8 si ottiene un offset allineato agli 8 byte.

				shr $3, %rax
				mov %rax, %rbx
				and $15, %rbx

Lo shift a destra di 3 bit equivale ad una divisione intera per 8 sull’offset in modo poi da ottenere in `rbx` il risultato di un’operazione modulo 16, effettuata tramite and bit a bit con 15. In `rbx` a questo punto è presente l’indice da utilizzare per settare il bit corrisponde nella word in **M** che segnala lo snapshot della quadword. Ora va individuata la word corrispondente in cui settare il bit.

				shr $4, %rax
				bts %bx, 8192(%rcx, %rax, 2)

Viene effettuato un ulteriore shift a destra di 4 bit in modo da avere in `rax` l’offset diviso per 128, questo corrisponde alla word in **M** dove va settato il bit individuato in precedenza.
L’operazione BTS permette di settare un bit a 1 e porre in CF il valore precedente. L’indirizzo della word è `8192 + rcx + rax * 2`, la moltiplicazione per 2 ovviamente è necessaria perché in **M** si opera sulle word.


				jc next_qword
				shl $4, %rax
				add %rbx, %rax
				mov (%rcx, %rax, 8), %rbx
				mov %rbx, 4096(%rcx, %rax, 8)

Nel caso in cui **CF** sia uguale a 1 si procede alla quadword successiva, ma proseguiamo con il caso **CF** uguale a 0.
Lo shift a sinistra ovvero la moltiplicazione per 16, insieme alla somma con `rbx`, che contiene l’indice del bit settato in M, sono necessarie alla ricostruzione dell’indirizzo di **A** “allineato”, ovvero successivo all’operazione `and $-8, %rax` .
L’indirizzo ottenuto da `rcx + rax * 8` è quello della quadword detta allineata. Seguono le operazioni che permettono di salvare il contenuto della quadword in `rbx` e successivamente in **S**.

			next_qword:
				mov %%gs:0, %%rax
				and $0xFF8, %%rax
				add $8, %rax
				cmp %rax, $4096
				jge end

Per ottenere l’offset della quadword successiva si parte di nuovo da `addressA` prelevandolo da **TLS**. L’operazione di and permettere di ottenere l’offset allineato verificato in precedenza a cui aggiungere 8 per passare alla quadword successiva. Prima di proseguire va verificato se l’offset ottenuto è al di fuori di **A**.

			second_qword:
				shr $3, %rax
				mov %rax, %rbx
				and $15, %rbx
				shr $4, %rax
				bts %bx, 8192(%rcx, %rax, 2)
				jc end
				shl $4, %rax
				add %rbx, %rax
				mov (%rcx, %rax, 8), %rbx
				mov %rbx, 4096(%rcx, %rax, 8)
			end:
				mov %gs:0, %rax
				mov %gs:8, %rbx
				mov %gs:16, %rcx

L’esecuzione prosegue replicando i passi visti in precedenza e termina portando alla condizione iniziale `rax`, `rbx` e `rcx`.

Il numero di accessi in memoria, in lettura e scrittura, effettuati dall’algoritmo è 13 nel caso l'offset non sia allineato.