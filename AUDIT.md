# Audit du projet malloc

Audit du 15/10/2026 sur `f4r10s12.paris.42.school` : Fedora, Linux 7.1.10, x86_64, GCC 16.2.1,
glibc, valgrind 3.27.1. Le dépôt n'a reçu aucune modification de code. Tous les correctifs
proposés ici ont été appliqués et testés sur une **copie** du projet dans un répertoire temporaire.

Les programmes de test écrits pour l'audit (scénarios, injection de fautes, stress, fork…) se
trouvent dans le répertoire temporaire de la session et seront perdus. Les reproducteurs
importants sont recopiés en entier dans ce rapport.

---

## 1. Verdict

**Oui, la partie obligatoire passe la correction en l'état**, à une condition préalable :
**le dépôt n'a encore aucun commit** (voir B1). Sans push sur la vogsphere, c'est 0.

- Tous les points de la fiche sont OK : nom de la bibliothèque, lien symbolique, export, test0 à
  test6, 268 pages pour test1 (note 5/5), test2 − test0 = 2 pages, `realloc`, erreurs,
  `show_alloc_mem`.
- Aucune fuite de l'allocateur. Chaque `mmap` a son `munmap`, y compris quand un `mmap`
  échoue au milieu d'une opération. Il ne reste en mémoire que 2 zones de réserve et le tableau
  des zones, ce qui est voulu et borné (voir §4.3).
- Aucun plantage dans les cas limites, le stress test (400 000 opérations), les tests
  multi-threads ni les vrais programmes (`ls`, `git`, `vim`, `python3`, `gcc`).
- Bonus : tout ce qu'annonce le README fonctionne.

Deux défauts **importants** restent à corriger avant la soutenance, parce qu'un correcteur
exigeant peut les trouver :

1. **I1** : tous les symboles internes sont exportés et substituables. Un programme ou une
   bibliothèque qui définit un `type_of()` ou un `hexdump()` fait **planter** le processus sous
   `LD_PRELOAD` (segfault démontré). Le sujet dit : *« In no way can your functions lead to
   undefined behavior or segfaults »*.
2. **I2** : **interblocage après `fork()`** dans un programme multi-threadé. 1 069 enfants sur
   3 000 restent bloqués à vie. Cela touche directement le bonus thread-safety.

Ces deux correctifs tiennent en une dizaine de lignes chacun et sont testés (diffs plus bas).

---

## 2. Problèmes classés par gravité

### Bloquant

#### B1. Le dépôt ne contient aucun commit

Preuve :

```
$ git log --oneline -1
fatal: your current branch 'master' does not have any commits yet
$ git status --short
?? .gitignore
?? Makefile
?? README.md
?? includes/
?? srcs/
$ git remote -v
origin  git@vogsphere.42paris.fr:vogsphere/intra-uuid-…-dasamuel (push)
```

La fiche dit : *« There is something in the git repository »*, et *« Only the work inside your
repository will be evaluated »*. Il faut committer et pusher, puis vérifier avec un clone frais
(`git clone … /tmp/verif && cd /tmp/verif && make`).

Attention aussi : `AUDIT.md` n'est pas ignoré. Ne le pushe pas si tu ne veux pas qu'il soit
rendu.

### Important

#### I1. Symboles internes exportés et substituables : segfault avec certains programmes

**Fichier** : [Makefile:10](Makefile#L10). Les 44 fonctions ont la visibilité par défaut.

La bibliothèque exporte 44 symboles, dont `type_of`, `align_up`, `hexdump`, `heap_init`,
`ft_memcpy`, `zone_of`… Comme elle est compilée en `-fPIC` sans contrôle de visibilité, **chaque
appel interne passe par la PLT**, donc il peut être redirigé vers une autre définition :

```
$ objdump -d libft_malloc.so | grep -E 'call.*<(heap|block|zone|type_of|ft_)[a-z_]*@plt>' | head -3
    call   …  <heap_init@plt>
    call   …  <block_find_free@plt>
    call   …  <zone_new_small@plt>
```

Il y a deux conséquences, démontrées toutes les deux.

**a) Un programme qui exporte une fonction du même nom détourne l'allocateur.**

```c
/* interpose.c */
int type_of(size_t x) { return (int)(x % 3) == 0 ? 2 : 0; }
int main(void) { char *p = malloc(20000); memset(p, 0x78, 20000); puts("survived"); free(p); return type_of(3); }
```

```
$ gcc -rdynamic -o interpose interpose.c && ./interpose          # glibc
survived                                                          (exit 2)
$ LD_PRELOAD=./libft_malloc.so ./interpose
Segmentation fault (core dumped)                                  (exit 139)
```

Notre `heap_alloc` appelle le `type_of` du programme. La requête de 20 000 octets part alors en
TINY, et le `memset` déborde de la zone.

**b) Nos symboles écrasent ceux des autres bibliothèques.** Une bibliothèque préchargée passe
avant les dépendances du programme :

```c
/* libfoo.so */  void hexdump(const char *title) { printf("libfoo hexdump: %s\n", title); }
/* usefoo.c  */  int main(void) { hexdump("hello"); return 0; }
```

```
$ LD_LIBRARY_PATH=. ./usefoo
libfoo hexdump: hello
$ LD_LIBRARY_PATH=. LD_PRELOAD=./libft_malloc.so ./usefoo
Segmentation fault (core dumped)
```

**Correction** : un version script pour le linker, qui n'exporte que l'API publique. Les appels
internes deviennent alors des appels directs (0 appel interne via la PLT), sans perte de
performance (stress : 2,70 s contre 2,76 s, dans le bruit de mesure).

```diff
--- /dev/null
+++ exports.map
@@ -0,0 +1,8 @@
+{
+	global:
+		malloc; free; realloc; calloc; reallocarray; malloc_usable_size;
+		memalign; posix_memalign; aligned_alloc; valloc;
+		show_alloc_mem; show_alloc_mem_ex;
+	local:
+		*;
+};
--- a/Makefile
+++ b/Makefile
@@ -10 +10 @@
-LDFLAGS		= -shared
+LDFLAGS		= -shared -Wl,--version-script=exports.map
@@ -43 +43 @@
-$(NAME): $(OBJS)
+$(NAME): $(OBJS) exports.map
```

Vérification sur la copie corrigée :

```
$ nm -D --defined-only libft_malloc.so | awk '{print $3}'
aligned_alloc calloc free malloc malloc_usable_size memalign posix_memalign realloc
reallocarray show_alloc_mem show_alloc_mem_ex valloc
internal PLT calls left: 0
$ LD_PRELOAD=./libft_malloc.so ./interpose        -> survived, exit=2
$ LD_PRELOAD=./libft_malloc.so ./usefoo           -> libfoo hexdump: hello, exit=0
stress 400 000 ops : corrupted=0, 0 erreur de structure ; mt 8 threads : corrupted=0
ls, git status, gcc, python3, sort : exit=0
```

`nm` montre toujours `T malloc`, `T free`, `T realloc` et `T show_alloc_mem` : le test « Functions
export » de la fiche reste OK.

#### I2. Interblocage après `fork()` dans un programme multi-threadé

**Fichier** : [srcs/core/globals.c:6](srcs/core/globals.c#L6)

`fork()` copie `g_mutex` dans l'état où il se trouve. Si un autre thread le tient à cet instant,
l'enfant hérite d'un mutex verrouillé par un thread qui n'existe pas chez lui, et son premier
`malloc` bloque pour toujours. glibc évite ce problème en verrouillant ses arènes autour du
`fork`.

Reproducteur :

```c
/* forkmt.c — gcc -O2 -fno-builtin forkmt.c -lpthread */
static volatile int stop;
static void *hammer(void *a) {
	unsigned x = (unsigned)(long)a * 2654435761u + 1;
	while (!stop) { x ^= x << 13; x ^= x >> 17; x ^= x << 5;
		char *volatile p = malloc(x % 900 + 1); p[0] = 1; free(p); }
	return NULL;
}
int main(void) {
	pthread_t t[4]; int hung = 0, ok = 0;
	for (long i = 0; i < 4; i++) pthread_create(&t[i], NULL, hammer, (void *)(i + 1));
	for (int i = 0; i < 3000; i++) {
		pid_t pid = fork();
		if (pid == 0) { char *volatile c = malloc(100); c[0] = 1; free(c); _exit(0); }
		int st, waited = 0;
		while (waitpid(pid, &st, WNOHANG) == 0 && waited < 200) { usleep(1000); waited++; }
		if (waited >= 200) { hung++; kill(pid, SIGKILL); waitpid(pid, &st, 0); } else ok++;
	}
	stop = 1; for (int i = 0; i < 4; i++) pthread_join(t[i], NULL);
	printf("forks: %d ok, %d child processes deadlocked (killed after 200 ms)\n", ok, hung);
}
```

```
$ ./forkmt                                       # glibc
forks: 3000 ok, 0 child processes deadlocked (killed after 200 ms)
$ LD_PRELOAD=./libft_malloc.so ./forkmt
forks: 1931 ok, 1069 child processes deadlocked (killed after 200 ms)
```

(`-fno-builtin` et `volatile` sont indispensables : sans eux, GCC supprime la paire
`free(malloc())` et le test ne prouve rien. C'est ce qui m'est arrivé au premier essai.)

Python avec des threads et `subprocess` ne déclenche pas le problème, parce que `subprocess`
n'appelle pas `malloc` entre `fork` et `exec`. N'importe quel programme C qui fait
`fork` puis `malloc` dans l'enfant le déclenche.

**Correction** : `pthread_atfork`, qui fait partie de libpthread et est donc autorisé. Il est
enregistré dans un constructeur, c'est-à-dire avant que le mutex puisse être tenu, parce que
`pthread_atfork` peut lui-même appeler `malloc`.

```diff
--- a/srcs/core/globals.c
+++ b/srcs/core/globals.c
@@ -30,3 +30,28 @@
 	g_heap.check = env_level("FT_MALLOC_CHECK");
 	g_heap.page_size = (size_t)page_size;
 }
+
+// fork() copies g_mutex as it is: if another thread held it, the child would
+// block forever on its first malloc. The lock is taken around fork() so the
+// heap is copied in a consistent state, then released on both sides.
+static void	fork_prepare(void)
+{
+	pthread_mutex_lock(&g_mutex);
+}
+
+static void	fork_parent(void)
+{
+	pthread_mutex_unlock(&g_mutex);
+}
+
+static void	fork_child(void)
+{
+	pthread_mutex_init(&g_mutex, NULL);
+}
+
+// Runs at load time, before any lock is held: pthread_atfork may call malloc
+__attribute__((constructor))
+static void	heap_atfork(void)
+{
+	pthread_atfork(fork_prepare, fork_parent, fork_child);
+}
```

Vérification sur la copie corrigée :

```
$ LD_PRELOAD=./libft_malloc.so ./forkmt          (2 lancements)
forks: 3000 ok, 0 child processes deadlocked (killed after 200 ms)
forks: 3000 ok, 0 child processes deadlocked (killed after 200 ms)
$ nm -D -u libft_malloc.so | grep -E 'atfork|mutex_init'
                 U pthread_mutex_init@GLIBC_2.2.5
                 U __register_atfork@GLIBC_2.3.2
```

Si on te demande en soutenance ce qu'est `__register_atfork` : sous glibc, `pthread_atfork` est
une petite fonction statique (de `libc_nonshared.a`) qui appelle `__register_atfork`. C'est bien
la fonction de libpthread autorisée par le sujet.

#### I3. test6 compilé exactement comme dans la fiche ne compile pas

La fiche donne `gcc -o test6 test6.c -L. -lft_malloc`, sans `-I`. Avec `#include "malloc.h"`, GCC
trouve alors le `malloc.h` de glibc (`/usr/include/malloc.h`), qui ne déclare pas
`show_alloc_mem`. Or depuis GCC 14, une déclaration implicite est une **erreur** :

```
$ gcc -o test6_fiche test6.c -L. -lft_malloc
test6.c:10:9: error: implicit declaration of function 'show_alloc_mem' [-Wimplicit-function-declaration]
gcc exit=1
$ gcc -I includes -o test6 tests/test6.c -L. -lft_malloc && ./run.sh ./test6     -> OK (voir §3)
```

Ce n'est pas un bug du projet. Le `test6.c` officiel n'est pas fourni (« link not available »), donc
je n'ai pas pu vérifier sa ligne `#include`. Prépare la commande `gcc -I includes …` pour le
correcteur, ou `-Wno-implicit-function-declaration` si son `test6.c` déclare la fonction
autrement.

#### I4. Divergences entre la fiche et le sujet (le sujet fait foi)

| Fiche | Sujet v6.3 | Conséquence |
|---|---|---|
| *« No norm errors, Norminette is authoritative »*, et la correction s'arrête sinon | *« Your project must be clean code even without the norm »* | La norme n'est pas exigée. Ton propre `make test` affiche `[KO] norminette : 36 fichier(s) en erreur`, ce qui sera la première chose vue si tu le lances devant le correcteur. Passe ce test en `[INFO]` dans `tests/correction.sh:50-59`, et prépare la citation du sujet (p. 3). |
| *« The author file is valid »* | Aucun fichier `author` demandé | Pas de fichier `author` dans le dépôt. Rien à faire. Si un correcteur insiste, montre la version 6.3 du sujet. |
| `nm` attend `U _getpagesize` | *« getpagesize under OSX or sysconf(_SC_PAGESIZE) under linux »* | `sysconf` est conforme. |
| `run.sh` : `$@` sans guillemets | — | Ton `tests/run.sh` utilise `"$@"`, c'est mieux. |
| test6 : `gcc … test6.c -L. -lft_malloc` | — | Voir I3. |
| La fiche ne parle que de malloc/free/realloc/show_alloc_mem | Le sujet ne demande pas d'autres fonctions | Les fonctions `calloc`, `memalign`… sont donc du bonus. |

#### I5. `make test` et le README renvoient à des fichiers qui ne seront pas rendus

**Fichiers** : [.gitignore:9](.gitignore#L9) (`tests/`), [Makefile:52](Makefile#L52),
[README.md:34](README.md#L34) et [README.md:42](README.md#L42).

Simulation d'un clone (uniquement les fichiers non ignorés) :

```
$ git ls-files --others --exclude-standard | tar cf - -T - | tar xf - -C clone && cd clone
$ make && make test
sh tests/correction.sh
sh: tests/correction.sh: No such file or directory
make: *** [Makefile:53: test] Error 127
```

Un correcteur qui lit le README et tape `make test` tombe sur une erreur. Choisis entre deux options :

- tu rends les tests (retire `tests/` de `.gitignore`, avec `tests/bin/` toujours ignoré) ;
- tu retires la règle et les deux mentions du README :

```diff
--- a/Makefile
+++ b/Makefile
-bonus: all
-
-test:
-	sh tests/correction.sh
+bonus: all
@@
-.PHONY: all bonus test clean fclean re
+.PHONY: all bonus clean fclean re
```

(et `clean` peut alors ne plus supprimer `tests/bin`.)

### Mineur

#### M1. Le lien symbolique n'est pas mis à jour quand `HOSTTYPE` change sans `re`

**Fichier** : [Makefile:38-41](Makefile#L38-L41)

```
$ make fclean && make && HOSTTYPE=Testing make
ln -sf libft_malloc_Testing.so libft_malloc.so
$ make
make: Nothing to be done for 'all'.
$ ls -l libft_malloc.so
libft_malloc.so -> libft_malloc_Testing.so          <- devrait pointer vers x86_64_Linux
```

La cible `$(LINK)` est un lien, donc make compare la date du fichier pointé
(`libft_malloc_Testing.so`, plus récent), et rien n'est refait. La fiche fait `make re`, donc elle
n'est pas touchée, mais c'est facile à corriger :

```diff
--- a/Makefile
+++ b/Makefile
-all: $(LINK)
-
-$(LINK): $(NAME)
-	ln -sf $(NAME) $(LINK)
+all: $(NAME)
+	@[ "$$(readlink $(LINK))" = "$(NAME)" ] || ln -sfn $(NAME) $(LINK)
```

Vérifié sur la copie : le lien suit `HOSTTYPE` dans les 4 enchaînements testés, et le second
`make` ne refait aucun lien de la bibliothèque.

#### M2. `errno` jamais positionné, `posix_memalign` renvoie `EINVAL` au lieu de `ENOMEM`

**Fichier** : [srcs/api/memalign.c:33-34](srcs/api/memalign.c#L33-L34)

```
errno after malloc(SIZE_MAX) = 0 (not set)
posix_memalign(16, SIZE_MAX) = 22 (EINVAL, POSIX says ENOMEM)
```

Le sujet ne demande que *« return a NULL pointer »*, donc c'est conforme. POSIX demande en
plus `errno = ENOMEM`. Le correctif pour `posix_memalign` ne coûte rien (pas de nouvel import) :

```diff
--- a/srcs/api/memalign.c
+++ b/srcs/api/memalign.c
@@ -31,6 +31,6 @@ int	posix_memalign(void **memptr, size_t alignment, size_t size)
 		return (EINVAL);
 	ptr = memalign(alignment, size);
 	if (!ptr)
-		return (EINVAL);
+		return (ENOMEM);
```

Positionner `errno` dans `malloc` importerait `__errno_location`, une fonction non listée dans
le sujet. Je ne le recommande pas sans pouvoir le justifier en bonus.

#### M3. `pvalloc` n'est pas remplacé (le README dit le contraire)

[README.md:27](README.md#L27) affirme : *« No glibc pointer can then reach our free »*. La doc
glibc (*Replacing malloc*) liste aussi `pvalloc`. Un `pvalloc` part donc chez glibc, et notre
`free` l'ignore. Ce n'est qu'une fuite silencieuse, sans plantage :

```
pvalloc(100) = 0x3c05b000 (not replaced: comes from glibc)
[ft_malloc] *** error in free(0x3C05B000): pointer was not allocated by malloc
```

Correctif à ajouter dans `srcs/api/memalign.c`, plus `pvalloc;` dans `exports.map` si I1 est
appliqué :

```diff
+void	*pvalloc(size_t size)
+{
+	size_t	page;
+
+	page = (size_t)sysconf(_SC_PAGESIZE);
+	if (size > SIZE_MAX - page)
+		return (NULL);
+	return (memalign(page, (size + page - 1) / page * page));
+}
```

#### M4. `show_alloc_mem_ex` : la plage affichée ne correspond pas à la taille annoncée

**Fichier** : [srcs/show/show_alloc_mem_ex.c:10](srcs/show/show_alloc_mem_ex.c#L10)

```
0x7F3FEDA8B040 - 0x7F3FEDA8B060 : 24 bytes used        <- la plage fait 32 octets
0x7F3FEDA89040 - 0x7F3FEDA8A000 : 2000 bytes used      <- la plage fait 4032 octets
```

La plage utilise `block->size` (la capacité), alors que le libellé indique `block->req` (la
taille demandée). C'est voulu, mais un correcteur qui compare avec `show_alloc_mem` peut croire à
une erreur. Une piste :

```diff
-	ft_putstr_fd(" : ", 1);
-	ft_putnbr_fd(block->req, 1);
-	ft_putstr_fd(" bytes used\n", 1);
+	ft_putstr_fd(" : ", 1);
+	ft_putnbr_fd(block->req, 1);
+	ft_putstr_fd(" / ", 1);
+	ft_putnbr_fd(block->size, 1);
+	ft_putstr_fd(" bytes used\n", 1);
```

#### M5. Valgrind remplace notre malloc quand le programme est lié avec `-lft_malloc`

C'est une information, pas un bug. La bibliothèque n'a pas de SONAME (`readelf -d` : `no
SONAME`). Or valgrind intercepte malloc dans les objets sans SONAME, comme pour l'exécutable :

```
$ FT_MALLOC_VERBOSE=1 valgrind --tool=helgrind ./mt_small 2>&1 | grep -c '^\[ft_malloc\]'
0                                    <- c'est le malloc de valgrind qui tourne, pas le nôtre
$ … valgrind --tool=helgrind --soname-synonyms=somalloc=nouserintercepts ./mt_small …
12380                                <- là, c'est bien le nôtre
```

Si un correcteur lance valgrind sur un test lié à ta bibliothèque, il teste en réalité le malloc
de valgrind. Explique-le-lui, et donne-lui l'option `--soname-synonyms=somalloc=nouserintercepts`.

#### M6. `abort()` appelé avec le mutex tenu (`FT_MALLOC_CHECK=2`)

[srcs/debug/debug.c:72](srcs/debug/debug.c#L72). C'est acceptable, puisque le processus va
mourir. Mais si le programme installe un gestionnaire de `SIGABRT` qui appelle `malloc`, il
s'interbloque. Pour être propre, il faudrait déverrouiller avant `abort()`, ou relever l'erreur
et laisser l'appelant faire `abort` après `pthread_mutex_unlock`. Ce point n'a pas été vérifié
par une exécution.

---

## 3. Fiche de correction

Les binaires de test sont compilés avec `gcc -o testN tests/testN.c`, et test6 avec `-I includes`.
Les mesures de pages sont faites avec `/usr/bin/time -v` (la commande GNU, pas le mot-clé de bash).

| # | Point de la fiche | Résultat | Preuve |
|---|---|---|---|
| 1 | Quelque chose dans le dépôt git | **KO** | Aucun commit (B1) |
| 2 | Fichier author | Obsolète | Le sujet v6.3 ne le demande pas (I4) |
| 3 | Makefile, règles usuelles | OK | `all`, `clean`, `fclean`, `re` présentes. Second `make` : `Nothing to be done for 'all'`. Après `touch srcs/utils/utils.c` : 1 recompilation + 1 lien. Après `touch srcs/core/types.h` : 16 recompilations (dépendances `-MMD`) |
| 4 | Norminette | Obsolète | Non exigée par le sujet (I4) |
| 5 | Fonctions interdites | OK, à justifier | Voir §5.2 : `getenv` et `abort` relèvent du bonus |
| 6 | 2 globales | OK | `nm` : seulement `B g_heap` et `B g_mutex`, aucune `static` de fichier ou de fonction |
| 7 | `HOSTTYPE=Testing make re` + lien | OK | `libft_malloc.so -> libft_malloc_Testing.so` |
| 8 | Export (nm) | OK | `T malloc`, `T free`, `T realloc`, `T show_alloc_mem` (+ `show_alloc_mem_ex`, `calloc`…) |
| 9 | Script `run.sh` | OK | `tests/run.sh` : `LD_LIBRARY_PATH=.`, `LD_PRELOAD=libft_malloc.so`, `"$@"` |
| 10 | test0 baseline | OK | 72 pages (glibc : 66) |
| 11 | test1 : pages consommées | **5/5** | 340 − 72 = **268 pages** (fourchette 255–272) |
| 12 | Zones pré-allouées ≥ 100 × la taille max, multiples de la page | OK | TINY 16 384 o = 4 pages, **102** blocs de 128 o mesurés ; SMALL 106 496 o = 26 pages, **100** blocs de 1 024 o mesurés |
| 13 | test2 : free fonctionne (moins de pages que test1) | OK | 74 < 340 |
| 14 | Qualité du free (test2 ≤ test0 + 3) | OK | 74 − 72 = **2** |
| 15 | test3 realloc | OK | `Bonjour\nBonjour`, exit 0 |
| 16 | test4 realloc++ | OK | `Bonjour\nBonjour`, exit 0 |
| 17 | test5 erreurs | OK | `Bonjour`, exit 0 |
| 18 | test6 show_alloc_mem | OK (avec `-I includes`) | Voir ci-dessous ; sans `-I`, ne compile pas (I3) |
| 19 | Bonus : accès concurrents (4 cas) | OK, **5/5** probable | Mutex dans `malloc` (`locked_alloc`), `free`, `realloc` et `show_alloc_mem`. Aucune course détectée par helgrind, drd ni TSan. Attention à I2 (fork) |
| 20 | Bonus additionnels | OK | Défragmentation, variables de debug, hexdump, historique (§6) |

### Mesure des pages (point 11 et 14) : attention au bruit de l'ASLR

En un seul lancement, comme le fera le correcteur, l'écart varie de quelques pages :

```
$ for k in $(seq 15); do … t1-t0, t2-t0 …; done | sort | uniq -c
      1 t1-t0=264 t2-t0=-5
      …
      1 t1-t0=268 t2-t0=6          <- 6 > 3 : « No » sur la qualité du free, par pur bruit
      2 t1-t0=269 t2-t0=3
```

Avec l'ASLR désactivée, c'est parfaitement stable :

```
$ setarch x86_64 -R ./run.sh /usr/bin/time -v ./test0   -> 72 72 72 72 72 72 72 72 72 72
$ setarch x86_64 -R ./run.sh /usr/bin/time -v ./test1   -> 340 (×10)
$ setarch x86_64 -R ./run.sh /usr/bin/time -v ./test2   -> 74 (×10)
```

Si le correcteur trouve 4, 5 ou 6 pages pour test2 − test0, propose-lui de relancer avec
`setarch $(uname -m) -R` (la fiche autorise *« another method of counting allocated pages »*).
Explication des 2 pages : la page du tableau des zones, plus la première page de la zone SMALL
(en-tête + bloc).

### test6

```
$ gcc -I includes -o test6 tests/test6.c -L. -lft_malloc && ./tests/run.sh ./test6
LARGE : 0x7F65C684A000
0x7F65C684A040 - 0x7F65CE84A040 : 134217728 bytes
LARGE : 0x7F65CE84B000
0x7F65CE84B040 - 0x7F65CF84B040 : 16777216 bytes
LARGE : 0x7F65CF84C000
0x7F65CF84C040 - 0x7F65CF94C040 : 1048576 bytes
SMALL : 0x7F65CF94D000
0x7F65CF94D040 - 0x7F65CF94D440 : 1024 bytes
LARGE : 0x7F65CFB71000
0x7F65CFB71040 - 0x7F65CFB79040 : 32768 bytes
Total : 152077312 bytes
```

Le format est conforme à l'exemple du sujet :

- l'en-tête est `TYPE : 0xADRESSE` ;
- chaque ligne est `debut - fin : N bytes`, avec `fin = debut + N` (0x440 − 0x040 = 1 024) ;
- les adresses sont en hexadécimal **majuscule** avec `0x`, comme dans l'exemple ;
- la ligne finale est `Total : N bytes` ;
- l'ordre est par **adresses croissantes**, comme l'exige le sujet.

Les types sont donc mélangés (LARGE, SMALL, LARGE). Un correcteur qui s'attend à « TINY puis
SMALL puis LARGE », parce que c'est l'ordre de l'exemple, peut tiquer. Réponds-lui avec la
phrase du sujet : *« The visual will be formatted by increasing addresses »*. Les zones vides (la
réserve) ne sont pas affichées.

---

## 4. Fuites

### 4.1 Programmes de test sous valgrind (compilés contre la libc normale)

Commande : `valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all ./X Bonjour`.
Pour test6, edge, debug et threads, un stub `show_alloc_mem` est ajouté, puisque glibc n'en a pas.

| Programme | En usage à la sortie | Erreurs | Analyse |
|---|---|---|---|
| test0 | 0 o | 0 | OK |
| test1 | 1 048 576 o / 1 024 blocs (definitely 1 047 552 + possibly 1 024) | 2 | **Fuite voulue par la fiche** : test1 mesure la mémoire consommée et ne doit pas libérer |
| test2 | 0 o | 0 | OK |
| test3 | 134 217 728 o (possibly) | 1 | Test officiel, sans `free` |
| test4 | 150 994 944 o (possibly) | 2 | Test officiel, sans `free` |
| test5 | 16 o (definitely) + 2 « Invalid free » | 3 | Test officiel. Les `free(addr + 5)` et `realloc(addr + 5)` invalides **sont** le test |
| test6 | 152 077 312 o / 5 blocs | 5 | Test officiel, sans `free` (sinon `show_alloc_mem` n'aurait rien à montrer) |
| bonus/edge | **0 o** | 6 « Invalid free » | Voulu : `free(stack)`, `free(ptr + 8)`, double `free`… |
| bonus/debug | **0 o** | 1 « Invalid free » | Voulu : double `free(a)` pour tester `FT_MALLOC_CHECK` |
| bonus/threads | **0 o** | 0 | OK (`__libc_freeres` vide le cache de piles de glibc) |

**Conclusion** : tes propres tests (edge, debug, threads) ne perdent **aucun octet**, et n'ont
aucun bloc « still reachable ». Les « fuites » de test1/3/4/5/6 sont celles des **tests officiels
de la fiche**. Les corriger changerait ce que la fiche mesure (pages de test1, affichage de test6).
Garde-les identiques aux originaux.

### 4.2 L'allocateur lui-même (valgrind ne voit pas les `mmap`)

**a) Équilibre `mmap`/`munmap` sous strace.** Le scénario fait 3 000 allocations
TINY/SMALL/LARGE libérées dans l'ordre, puis 3 000 tailles mêlées libérées à l'envers, 200 chaînes
de `realloc` de 1 à 300 000 octets, et 500 tours de `calloc`, `reallocarray`, `memalign(64)`,
`posix_memalign(4096)`, `valloc`, `aligned_alloc(8192)` et `realloc(p, 0)`, plus les chemins
d'échec. Les appels sont appariés **adresse par adresse et taille par taille** entre les
marqueurs BEGIN et END :

```
$ LD_PRELOAD=./libft_malloc.so strace -e trace=mmap,munmap,write -o scen.strace ./scen
mmap calls between BEGIN/END: 6790
munmap calls: 6787
$ python3 balance.py mid.strace
mmaps never unmapped: 3
  0x7f361be31000  106496 bytes (26 pages)     <- réserve SMALL
  0x7f361bec4000  16384 bytes (4 pages)       <- réserve TINY
  0x7f361bf9b000  16384 bytes (4 pages)       <- tableau des zones (a grandi à 2 048 entrées)
```

Aucun `munmap` n'a une taille différente de celle de son `mmap`.

**b) `show_alloc_mem_ex()` après tout libéré :**

```
SMALL : 0x7F361BE31000 (106496 bytes mapped)
0x7F361BE31040 - 0x7F361BE4B000 : free (106432 bytes)
TINY : 0x7F361BEC4000 (16384 bytes mapped)
0x7F361BEC4040 - 0x7F361BEC8000 : free (16320 bytes)
Total : 0 bytes used, 122752 bytes free, 2 zones
```

Il n'y a aucune zone orpheline, et chaque zone restante est **un seul** bloc libre.

**c) Échec d'un `mmap` au milieu d'une opération.** Une bibliothèque d'injection, préchargée avant
la nôtre, fait échouer le N-ième `mmap` et compte les mappings vivants :

```
$ LD_PRELOAD="./libfail.so ./libft_malloc.so" ./failinj
baseline live mappings (table + tiny spare): 2
A  LARGE, zone mmap fails          -> (nil), mappings delta 0
B  LARGE, table_grow mmap fails     -> (nil), mappings delta 0 (0 = zone munmapped)
B' same call without failure         -> ok, mappings delta 1 (zone + new table - old table = 1)
C  TINY, new zone mmap fails       -> NULL after 204 tiny allocs, mappings delta 0
D  realloc must move, mmap fails    -> (nil), old still 'survivor', mappings delta 0
E  memalign(4096), mmap fails       -> (nil), mappings delta 0
F  calloc LARGE, mmap fails         -> (nil), mappings delta 0
after freeing everything: 2 live mappings (table + tiny spare = 2 expected)
Total : 0 bytes
```

Le cas B est le plus délicat : la zone est créée, puis l'agrandissement du tableau échoue.
`zone_new` fait bien `munmap` de la zone ([srcs/zone/zone.c:36-40](srcs/zone/zone.c#L36-L40)), et
`table_grow` laisse l'ancien tableau intact.

**d) memcheck avec notre allocateur actif** (il détecte des lectures non initialisées ou hors
limites dans le code de l'allocateur) :

```
$ valgrind --trace-children=yes --soname-synonyms=somalloc=nouserintercepts \
    env LD_PRELOAD=$PWD/libft_malloc.so ./scen
total heap usage: 0 allocs, 0 frees      <- valgrind ne voit rien : c'est bien notre malloc qui tourne
ERROR SUMMARY: 0 errors from 0 contexts
```

**e) Limites mémoire du processus** : le noyau applique `RLIMIT_AS` et, depuis Linux 4.7,
`RLIMIT_DATA` aux `mmap` anonymes privés. `malloc` renvoie alors NULL proprement :

```
$ (ulimit -v 200000; LD_PRELOAD=./libft_malloc.so ./misc rlimit)
malloc(1MB) returned NULL cleanly after 192 MB
small malloc still works after hitting the limit: yes
$ (ulimit -d 100000; LD_PRELOAD=./libft_malloc.so ./misc rlimit)
malloc(1MB) returned NULL cleanly after 97 MB
small malloc still works after hitting the limit: yes
```

### 4.3 Ce qui reste mappé à la fin : la réserve et le tableau des zones

**Ce qui reste** : au plus 2 zones de réserve (16 KB + 104 KB) et le tableau des zones (4 KB,
plus s'il a grandi, car il ne rétrécit jamais), soit **≈ 124 KB, bornés**, quel que soit
l'historique du programme.

**Un correcteur strict peut-il parler de fuite « reachable » ?** À mon avis, non, et voici les
arguments à donner :

- **Valgrind ne peut pas le voir.** Le « still reachable » de valgrind compte les blocs que le
  *programme* a obtenus de `malloc` et n'a pas rendus. Ces mappings appartiennent à l'allocateur.
  Ils sont invisibles pour memcheck (voir 4.2.d : `0 allocs`), et `show_alloc_mem` ne les
  affiche pas (zones vides).
- **glibc fait pareil.** Sa *main arena* (le tas `brk`), son *top chunk* et le cache tcache ne
  sont jamais rendus à la sortie. La réserve, c'est exactement ce que demande le sujet : *« limit
  the number of calls to mmap(), but also to munmap() »*.
- **C'est atteignable et borné**, pas perdu : `g_heap.spare[]` et `g_heap.zones` pointent
  dessus jusqu'à la fin, et le noyau récupère tout à `exit`.

**Si tu veux quand même un destructeur**, voici une version testée :

```diff
--- a/srcs/core/heap.c
+++ b/srcs/core/heap.c
@@ -2,6 +2,7 @@
 #include "core/core.h"
 #include "utils/utils.h"
 #include "zone/zone.h"
+#include <sys/mman.h>
@@ -121,3 +122,33 @@
 	freelist_push(block, type);
 	release_if_empty(loc->zone);
 }
+
+// Gives the spare zones back and, once nothing is allocated, the zone table.
+// Other destructors may still call malloc or free afterwards: the state stays
+// consistent, a later malloc just maps again. trylock: exit() may be called
+// from a signal handler that interrupted malloc in this very thread.
+__attribute__((destructor))
+static void	heap_cleanup(void)
+{
+	int	type;
+
+	if (pthread_mutex_trylock(&g_mutex))
+		return ;
+	type = TINY;
+	while (type <= SMALL)
+	{
+		if (g_heap.spare[type])
+		{
+			freelist_remove(g_heap.spare[type]->blocks, type);
+			zone_release(g_heap.spare[type]);
+			g_heap.spare[type] = NULL;
+		}
+		type++;
+	}
+	if (!g_heap.count && g_heap.zones)
+	{
+		munmap(g_heap.zones, g_heap.capacity * sizeof(t_zone *));
+		g_heap.zones = NULL;
+		g_heap.capacity = 0;
+	}
+	pthread_mutex_unlock(&g_mutex);
+}
```

**Résultat** : avec le scénario de 4.2.a, `mmaps never unmapped: 0`.

**Les risques, et comment cette version les traite** :

| Risque | Traitement | Preuve |
|---|---|---|
| L'ordre des destructeurs entre bibliothèques n'est pas garanti : un autre destructeur (ou un handler `atexit`, ou un destructeur C++) peut appeler `free` ou `malloc` **après** le nôtre | Le destructeur ne détruit que ce qui est vide, et laisse l'état cohérent : les zones vivantes restent dans le tableau, et `spare = NULL`. Le tableau n'est rendu que si `count == 0`, auquel cas un `free` tardif ne trouve aucune zone et est ignoré. Un `malloc` tardif recrée simplement zone et tableau | Test simulant les deux ordres : `after cleanup #1: still alive`, puis `free` des blocs vivants, puis `free` de pointeurs obsolètes après disparition du tableau (ignorés et signalés), puis `malloc after cleanup works`, `Total : 0 bytes` |
| `exit()` appelé depuis un gestionnaire de signal qui a interrompu `malloc` **dans le même thread** : avec `pthread_mutex_lock`, le destructeur bloque pour toujours | `pthread_mutex_trylock` : si le mutex est pris, le destructeur ne fait rien | Avec `lock` : **50/50** processus bloqués à la sortie. Avec `trylock` : 0/50. Sans destructeur (ta version actuelle) : 0/50 |
| Un autre thread est encore dans `malloc` pendant `exit()` | Le mutex, et `trylock` qui abandonne sans attendre | — |
| Le destructeur ne s'exécute pas avec `_exit`, `abort` ou un signal mortel | Sans importance : le noyau libère tout | — |
| `__attribute__((destructor))` est-il « autorisé » ? | C'est un attribut du compilateur, pas une fonction. Il n'apparaît pas dans `nm -u` | — |

**Ma recommandation** : le destructeur n'apporte **rien d'observable** à la correction. Le gain
est purement cosmétique (le noyau libère tout de toute façon), et il ajoute un code de sortie
délicat (voir le piège `lock`/`trylock`). Je le garderais comme réponse orale (« je pourrais, voici
les risques ») plutôt que de l'ajouter. Si tu l'ajoutes, prends la version `trylock` ci-dessus,
telle quelle.

### 4.4 `realloc`, `calloc`, `reallocarray`, `memalign`, `posix_memalign`, `valloc`

| Chemin | Comportement | Preuve |
|---|---|---|
| `realloc` qui réussit en place (grossir dans le voisin libre, rétrécir avec découpe) | Pas de copie, le reste est rendu à la liste libre | stress : 0 corruption, 0 erreur de structure |
| `realloc` qui déplace | Copie de `min(old_req, size)`, puis `heap_free` de l'ancien bloc | Équilibre strace 4.2.a |
| `realloc` qui échoue (taille > `SIZE_MAX/2` ou `mmap` refusé) | NULL, **l'ancien bloc reste valide et intact** | `D … old still 'survivor', mappings delta 0` |
| `realloc` d'un pointeur invalide (pile, milieu de bloc, déjà libéré) | NULL, rien n'est touché | edge2 : `realloc stack pointer`, `realloc middle of block -> NULL` |
| `calloc` qui déborde | NULL avant toute allocation | `calloc(SIZE_MAX/2, 3) -> NULL` |
| `calloc` qui échoue | NULL, rien n'est mappé | `F calloc LARGE, mmap fails -> (nil), mappings delta 0` |
| `reallocarray` qui déborde | NULL, l'ancien bloc est conservé | `reallocarray overflow -> NULL` |
| `memalign`, `aligned_alloc`, `posix_memalign`, `valloc` | Toujours LARGE (un `mmap` par appel), rendu au `free` | 500 tours du scénario 4.2.a équilibrés ; `E memalign(4096), mmap fails -> delta 0` |
| `posix_memalign` qui échoue | Renvoie EINVAL, `*memptr` n'est pas touché | Voir M2 (ENOMEM serait plus juste) |

---

## 5. Conformité au sujet

### 5.1 Makefile

| Exigence | Résultat |
|---|---|
| `libft_malloc_$HOSTTYPE.so` | OK : `libft_malloc_x86_64_Linux.so` |
| `HOSTTYPE` vide ou absente | OK : bloc `ifeq` identique au sujet. Sous bash, `$HOSTTYPE` vaut `x86_64` mais n'est **pas exportée**, donc make ne la voit pas et utilise `uname` : `x86_64_Linux`. Avec `export HOSTTYPE=Testing`, on obtient bien `libft_malloc_Testing.so` |
| Lien `libft_malloc.so` | OK (voir M1 pour le cas sans `re`) |
| `all`, `clean`, `fclean`, `re` | OK |
| Pas de relink inutile | OK : `make: Nothing to be done for 'all'` |
| `-Wall -Wextra -Werror` | OK avec gcc 16. Compile aussi sans avertissement avec `clang -Wall -Wextra -Werror` et `gcc -fanalyzer` |

### 5.2 Fonctions externes

```
$ nm -D -u libft_malloc.so
                 U abort@GLIBC_2.2.5
                 w __cxa_finalize@GLIBC_2.2.5
                 U getenv@GLIBC_2.2.5
                 w __gmon_start__
                 w _ITM_deregisterTMCloneTable
                 w _ITM_registerTMCloneTable
                 U mmap@GLIBC_2.2.5
                 U munmap@GLIBC_2.2.5
                 U pthread_mutex_lock@GLIBC_2.2.5
                 U pthread_mutex_unlock@GLIBC_2.2.5
                 U sysconf@GLIBC_2.2.5
                 U write@GLIBC_2.2.5
```

| Symbole | Autorisé ? | Justification pour la soutenance |
|---|---|---|
| `mmap`, `munmap` | Oui | Liste du sujet |
| `sysconf` | Oui | *« sysconf(_SC_PAGESIZE) under linux »*. Utilisé seulement avec `_SC_PAGESIZE` ([globals.c:25](srcs/core/globals.c#L25), [memalign.c:46](srcs/api/memalign.c#L46)) |
| `write` | Oui | *« authorized functions within your libft (write(2) for example) »* |
| `pthread_mutex_lock` / `unlock` | Oui | *« The functions from libpthread »* |
| `getenv` | **Bonus** | *« You are allowed to use other functions to complete the bonus part as long as their use is justified »*. Il lit `FT_MALLOC_SCRIBBLE`, `FT_MALLOC_VERBOSE` et `FT_MALLOC_CHECK`, ce qui est le bonus *« Manage malloc debug environment variables »*. On ne peut pas lire l'environnement autrement (sauf `/proc/self/environ` avec `open`/`read`, encore moins autorisés). `getenv` ne fait pas d'allocation, donc on peut l'appeler sous le mutex |
| `abort` | **Bonus** | Uniquement pour `FT_MALLOC_CHECK=2`, qui imite le `MALLOC_CHECK_=2` de glibc |
| `__cxa_finalize`, `__gmon_start__`, `_ITM_*` | — | Symboles faibles ajoutés automatiquement par GCC dans tout `.so`. Ils ne viennent pas de ton code |

Si tu appliques I2, s'y ajoutent `pthread_mutex_init` (libpthread) et `__register_atfork`
(c'est `pthread_atfork`, voir I2).

### 5.3 Globales

```
$ nm libft_malloc.so | grep -iE ' [bBdD] '        # hors symboles du runtime (completed.0, __dso_handle…)
00000000000061c0 B g_heap
0000000000006180 B g_mutex
```

Il y a exactement 2 globales, comme le sujet l'autorise : une pour les allocations, une pour le
thread-safety. Aucune variable `static`, ni globale ni locale (`grep static srcs` ne trouve que
des fonctions).

### 5.4 Prototypes de `malloc.h`

[includes/malloc.h](includes/malloc.h) : `void free(void *ptr);`, `void *malloc(size_t size);`,
`void *realloc(void *ptr, size_t size);`, `void show_alloc_mem(void);` et
`void show_alloc_mem_ex(void);`. Les trois premiers sont identiques au sujet. Pour
`show_alloc_mem`, le sujet écrit `()` et tu écris `(void)`, ce qui est équivalent et plus strict
en C. Les fonctions bonus (`calloc`, `memalign`…) ne sont pas déclarées dans `malloc.h`, mais
elles ont exactement les prototypes de `<stdlib.h>` et `<malloc.h>` de la libc (vérifié par la
compilation des tests avec ces en-têtes).

### 5.5 Alignement

*« You must properly align the memory returned by your malloc »*. Toutes les tailles de 0 à
2 999 donnent des adresses multiples de 16, avec `malloc_usable_size >= size`. `memalign`,
`aligned_alloc` et `posix_memalign` sont OK pour les alignements 16, 32, 64, 128, 4096, 8192 et
65536. `valloc` est aligné sur la page. Un alignement qui n'est pas une puissance de 2 donne NULL,
et `posix_memalign(4)` renvoie EINVAL.

### 5.6 `getrlimit`

Le sujet **autorise** `getrlimit(2)` sans l'**exiger**. La fiche n'en parle pas. Ton code ne
l'appelle pas, et ce n'est pas nécessaire : la limite est appliquée par le noyau au `mmap`
lui-même (`RLIMIT_AS`, `RLIMIT_DATA`), et `malloc` renvoie alors NULL proprement (preuve en
4.2.e). Appeler `getrlimit` ne ferait que dupliquer ce contrôle, en moins précis : il faudrait
connaître toute la mémoire du processus, pas seulement la nôtre. Réponse à donner si on te le
demande : « le noyau fait respecter la limite au `mmap`, je gère son échec ».

### 5.7 Cohérence de `types.h` et des `.c`

C'est cohérent. `t_heap` déclare `scribble`, `verbose` et `check` ([types.h:93-95](srcs/core/types.h#L93-L95)),
et c'est exactement ce que lisent `heap.c`, `realloc.c`, `calloc.c` et `debug.c`. Il n'existe
aucune trace d'un champ `opts` ni de constantes `OPT_*` (`grep -rn 'opts\|OPT_' srcs includes`
ne renvoie rien). La compilation avec `-Wall -Wextra -Werror` (gcc et clang) passe sans
avertissement.

---

## 6. Robustesse, threads, bonus

### 6.1 Cas limites (tous OK, aucun plantage)

```
OK    malloc(0) -> unique aligned pointer
OK    malloc(SIZE_MAX) -> NULL
OK    malloc(SIZE_MAX/2+1) -> NULL
OK    malloc(64 TiB) -> NULL (mmap refuses)
OK    calloc overflow -> NULL
OK    calloc(0, 10) -> pointer
OK    reallocarray overflow -> NULL
OK    realloc(NULL, 42) == malloc(42)
      realloc(p, 0) -> 0x7ffb72655040 (16-byte block, p freed)   <- libellé de mon test : en fait p est rétréci sur place, voir Q9
OK    free(NULL)
OK    double free TINY: no crash
OK    double free LARGE (zone unmapped): no crash
OK    free inside / header of a block: ignored
OK    free inside / header of a LARGE: ignored
OK    free/realloc stack pointer: ignored, realloc -> NULL
OK    free(0x1000), free(-16): ignored
OK    realloc middle of block -> NULL
OK    block still usable after invalid frees
OK    sizes 0..2999: aligned 16, usable_size >= size
Total : 0 bytes
```

Avec `FT_MALLOC_CHECK=1`, chaque cas est signalé avec le bon diagnostic : `double free`,
`pointer is not the start of a block` ou `pointer was not allocated by malloc`.

### 6.2 Fragmentation et fusion

Le stress test fait 400 000 opérations aléatoires (60 % TINY, 30 % SMALL, 10 % LARGE jusqu'à
200 KB ; `malloc`, `calloc`, `realloc` et `free`). Le contenu de chaque bloc est vérifié à chaque
accès, et celui de chaque `calloc` est vérifié à zéro. À 11 points de contrôle, un script analyse
`show_alloc_mem_ex` et vérifie :

- qu'il n'y a jamais deux blocs libres adjacents ;
- qu'il n'y a ni trou ni chevauchement (chaque bloc commence 32 octets après la fin du
  précédent) ;
- que la zone est couverte jusqu'à son dernier octet.

```
ops=400000 corrupted=0 nomem=0          (2,8 s ; glibc : 1,6 s)
checkpoints: 11 structural errors: 0
final state:
  SMALL : 0x7FC6A58BA000 (106496 bytes mapped)
  0x7FC6A58BA040 - 0x7FC6A58D4000 : free (106432 bytes)
  TINY : 0x7FC6A61B6000 (16384 bytes mapped)
  0x7FC6A61B6040 - 0x7FC6A61BA000 : free (16320 bytes)
  Total : 0 bytes used, 122752 bytes free, 2 zones
```

Une zone entièrement libérée redevient bien un seul bloc libre.

### 6.3 Vrais programmes sous `LD_PRELOAD`

| Commande | Résultat |
|---|---|
| `ls -la /usr/bin` | exit 0, 3 291 lignes. `FT_MALLOC_VERBOSE` compte 55 opérations : c'est bien notre malloc |
| `git status` | exit 0 |
| `vim` (pty) : ouvrir, insérer, `:wq` | exit 0, fichier écrit (`hello from vim`) |
| `vim` avec vimrc par défaut et `syntax on` sur un `.c` | exit 0 en 0,04 s |
| `python3 -c` (dict de 200 000 entrées + `json.dumps`) | exit 0 |
| `python3`, 4 threads + 50 `subprocess.run` | exit 0 |
| `gcc -O2 hello.c` | exit 0 |
| `sort -R /usr/share/dict/words` (480 000 lignes) | exit 0 |
| `find /usr/include -name '*.h'` | exit 0 |
| `bash -c 'for i in $(seq 200); do x=$(echo $i); done'` | exit 0 |

### 6.4 Thread safety

| Test | Résultat |
|---|---|
| `tests/bonus/threads.c` : 8 threads × 100 000 `malloc`/`realloc`/`free` + `show_alloc_mem` | `Total : 0 bytes`, 8,0 s |
| Stress multi-threads : 8 threads × 50 000 opérations sur 512 pointeurs **partagés** (alloués par un thread, vérifiés, réalloués et libérés par un autre), avec `malloc`, `calloc`, `memalign(64)` et `realloc` | `corrupted=0` sur 5 lancements |
| helgrind (avec `--soname-synonyms=somalloc=nouserintercepts`, voir M5) | `ERROR SUMMARY: 0 errors`, 12 380 opérations de **notre** malloc |
| drd (même option) | `ERROR SUMMARY: 0 errors` |
| TSan : sources de l'allocateur compilées avec `-fsanitize=thread` dans le test, symboles renommés en `ft_*` | Aucun `WARNING: ThreadSanitizer`, `Total : 0 bytes` |
| `fork()` avec des threads actifs | **KO, voir I2** |

Les 5 184 octets qui restent après le stress multi-threads ne sont pas une fuite de
l'allocateur. Ce sont le buffer de stdout (4 096 o) et les DTV de glibc pour les piles de
threads en cache (272 o chacun), que glibc ne libère qu'avec `__libc_freeres` :

```
$ ./pc        # 1 pthread_create + pthread_join
Total : 0 bytes
-- after 1 pthread_create/join:
SMALL : 0x7F16CE6A9000
0x7F16CE6A9040 - 0x7F16CE6A9150 : 272 bytes
```

**Aucun appel à malloc ou printf sous le mutex.** Voici toutes les fonctions externes appelées par
le code qui s'exécute sous le mutex, d'après `objdump -d`, appel PLT par appel PLT :
`mmap`, `munmap`, `write`, `getenv`, `sysconf` et `abort`. Aucune n'appelle `malloc`.
Le seul `malloc@plt` de la bibliothèque est dans `realloc`, et il est appelé **avant** le
verrouillage, pour le cas `realloc(NULL, n)`. `reallocarray`, `valloc`, `aligned_alloc` et
`posix_memalign` appellent `realloc` ou `memalign` sans tenir le mutex.

### 6.5 Bonus annoncés dans le README

| Bonus | Résultat | Preuve |
|---|---|---|
| Thread safety | OK, avec la réserve I2 | §6.4 |
| Défragmentation (fusion avec les voisins libres) | OK | §6.2 : 0 paire de blocs libres adjacents sur 11 points de contrôle |
| `show_alloc_mem_ex()` (blocs libres) | OK | `0x7F3FED861040 - 0x7F3FED87B000 : free (106432 bytes)` |
| Hexdump | OK (limité à 64 octets, puis `...`) | `0000  48 65 6C 6C 6F 20 6D 61 6C 6C 6F 63 21 00 00 00 \|Hello malloc!...\|` |
| Historique (64 dernières opérations) | OK, y compris quand il déborde | `History (207 operations, last 64 shown) :` de `#143` à `#206` ; un `realloc` raté est tracé `= 0x0` |
| `FT_MALLOC_SCRIBBLE=1` | OK | Allocation : `AA AA AA …`. `free` : `48/48 bytes == 0x55` (les 16 premiers octets contiennent les liens de la liste libre) |
| `FT_MALLOC_VERBOSE=1` | OK | 7 lignes `[ft_malloc] malloc(24) = 0x…`, `free(…) 24 bytes`, `realloc(…, 2000) = …` |
| `FT_MALLOC_CHECK=1` | OK | `[ft_malloc] *** error in free(0x7F7AA80ED040): double free`, exit 0 |
| `FT_MALLOC_CHECK=2` | OK | Même message, puis `Aborted`, exit 134 |
| Remplacement de `calloc`, `reallocarray`, `malloc_usable_size`, `memalign`, `posix_memalign`, `aligned_alloc` et `valloc` | OK | §5.5, §4.4. Manque `pvalloc` (M3) |

---

## 7. Questions probables en soutenance

**1. Quelles sont tes tailles n, m, N et M, et pourquoi ?**
TINY ≤ 128 o, SMALL ≤ 1 024 o ([types.h:10-11](srcs/core/types.h#L10-L11)). Une zone doit
contenir 100 blocs de taille maximale avec leur en-tête de 32 o, plus l'en-tête de zone. Pour
TINY, cela donne 32 + 100 × 160 = 16 032, arrondi à la page : 16 384 = 4 pages, soit 102 blocs
en pratique. Pour SMALL, 32 + 100 × 1 056 = 105 632, arrondi à 106 496 = 26 pages, soit 100
blocs ([zone.c:59-60](srcs/zone/zone.c#L59-L60)). Avec ces tailles, test1 coûte 268 pages pour
256 pages de données.

**2. Comment `free` sait-il qu'un pointeur est valide ?**
Les zones sont rangées dans un tableau trié par adresse. Une recherche dichotomique trouve la
zone candidate (`zone_of`, [zone_table.c:82](srcs/zone/zone_table.c#L82)). Ensuite, `heap_lookup`
parcourt les blocs de cette zone jusqu'à trouver un bloc dont les données commencent
**exactement** à `ptr` ([lookup.c:19](srcs/core/lookup.c#L19)). On obtient alors OK, FREED
(double free), INSIDE (milieu d'un bloc) ou NONE (pas à nous). Rien n'est jamais lu à une adresse
qui ne nous appartient pas, contrairement à un `ptr - sizeof(header)` naïf.

**3. Pourquoi 32 octets d'en-tête, et que contient-il ?**
`size` (la capacité), `req` (la taille demandée, utilisée par `show_alloc_mem` et le
`realloc` en place), `next` et `free` : 4 × 8 = 32, un multiple de 16, donc les données restent
alignées sur 16. Les liens de la liste libre (`t_link`) sont stockés **dans** les données des
blocs libres, ce qui ne coûte rien.

**4. Comment trouves-tu un bloc libre rapidement ?**
65 listes libres par type, une par multiple de 16 octets, la dernière pour tout ce qui est plus
gros ([freelist.c:44](srcs/block/freelist.c#L44)). Dans les listes exactes, tous les blocs
conviennent, donc on prend le premier. Seule la dernière liste demande un parcours. Le bloc trouvé
est découpé si le reste peut contenir un en-tête et 16 octets.

**5. Comment fonctionne la défragmentation ?**
Au `free`, on fusionne avec le suivant s'il est libre (`block_merge`), puis avec le précédent
(`loc->prev`, trouvé pendant la recherche du pointeur). Comme la fusion est systématique, deux
blocs libres ne sont jamais adjacents, donc un seul voisin de chaque côté suffit.

**6. Pourquoi garder une zone vide par type ?**
Pour éviter l'aller-retour `mmap`/`munmap` dans une boucle `malloc`/`free` (test2) : *« limit
the number of calls to mmap(), but also to munmap() »*. Les autres zones vides sont rendues tout
de suite (`release_if_empty`, [heap.c:81](srcs/core/heap.c#L81)). C'est pour ça que test2 ne
coûte que 2 pages de plus que test0.

**7. Ça ne fuit pas, alors, puisque la réserve n'est jamais rendue ?**
C'est borné (≈ 124 KB au maximum), atteignable (`g_heap` pointe dessus), et invisible pour
valgrind, puisque ce ne sont pas des blocs du programme. glibc garde aussi son arène. Voir §4.3,
avec le destructeur possible et son piège.

**8. Que fait `realloc` ?**
Si le bloc peut grossir en place (le voisin suivant est libre, assez grand, et le bloc reste du
même type) ou s'il rétrécit, il est redimensionné sur place et le reste est découpé. Sinon, on
alloue un nouveau bloc, on copie `min(ancienne taille demandée, nouvelle)`, puis on libère
l'ancien. Si l'allocation échoue, on renvoie NULL **sans toucher l'ancien bloc**. Un pointeur
invalide donne NULL : c'est ce que vérifie test5.

**9. Que renvoie `realloc(p, 0)` ? Et `malloc(0)` ?**
`malloc(0)` renvoie un bloc unique de 16 octets, qu'on peut passer à `free`. `realloc(p, 0)`
rétrécit `p` sur place à 16 octets et renvoie `p` (`payload_size(0)` vaut 16, donc
`resize_in_place` réussit toujours). glibc, elle, libère et
renvoie NULL. Les deux comportements sont permis : c'est *implementation-defined* en C17, et
indéfini en C23.

**10. Comment garantis-tu l'alignement de `memalign(4096)` ?**
Tout alignement supérieur à 16 passe par une zone LARGE avec `align − 16` octets de marge
([heap.c:18-19](srcs/core/heap.c#L18-L19)). Les données sont placées au premier multiple de
`align` après l'en-tête, et l'en-tête de bloc est mis juste avant. `free` retrouve la zone par la
dichotomie, même si l'en-tête n'est pas au début du mapping.

**11. Pourquoi remplacer `calloc`, `memalign`… si le sujet ne demande que malloc, free et realloc ?**
Sinon, un programme qui appelle `calloc` ou `posix_memalign` obtient un pointeur de glibc, et ce
pointeur arrive ensuite dans **notre** `free`. Il serait alors ignoré (fuite) ou, dans un
allocateur naïf, provoquerait un plantage. `vim`, `python3` et `git` utilisent ces fonctions.

**12. Comment as-tu rendu l'allocateur thread-safe ? Pourquoi un seul mutex ?**
Un mutex global (`g_mutex`, la 2ᵉ globale autorisée). Il couvre `malloc`, `free`, `realloc`,
`calloc`, `memalign`… et les deux `show_alloc_mem`. Comme il n'y a qu'un seul verrou, aucun ordre
de verrouillage n'est possible, donc pas d'interblocage entre verrous. Rien n'appelle `malloc` ou
`printf` sous le verrou : la sortie se fait avec `write`. Si tu as appliqué I2, ajoute
`pthread_atfork` pour `fork()`.

**13. Pourquoi `getenv` et `abort`, qui ne sont pas dans la liste ?**
Ils servent au bonus « variables de debug », que le sujet autorise sous réserve de
justification. Voir §5.2.

**14. Comment as-tu compté les pages ?**
Avec `/usr/bin/time -v`, ligne « Minor page faults » : 340 pour test1, 72 pour test0, donc 268
pages. L'ASLR fait varier la mesure de ±3 pages. `setarch -R` la rend stable (§3).
