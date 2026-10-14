#!/bin/sh
# Déroule la fiche de correction (malloc.pdf) étape par étape.
# Usage : sh tests/correction.sh   (ou make test)

cd "$(dirname "$0")/.." || exit 1

BIN=tests/bin
CC="cc -I includes"
BONUS_FLAGS="-Wall -Wextra -Wno-free-nonheap-object -Wno-use-after-free"
FAILS=0

if [ -t 1 ]; then
	GREEN='\033[32m'; RED='\033[31m'; YELLOW='\033[33m'
	BOLD='\033[1m'; RESET='\033[0m'
fi

title() { printf "\n${BOLD}== %s ==${RESET}\n" "$1"; }
ok()    { printf "${GREEN}[OK]${RESET}   %s\n" "$1"; }
ko()    { printf "${RED}[KO]${RESET}   %s\n" "$1"; FAILS=$((FAILS + 1)); }
info()  { printf "${YELLOW}[INFO]${RESET} %s\n" "$1"; }

# Plus petit nombre de "page reclaims" sur 10 lancements. L'ASLR fait varier
# la mesure de quelques pages : on le désactive avec setarch -R si possible.
SETARCH=
if setarch "$(uname -m)" -R true 2>/dev/null; then
	SETARCH="setarch $(uname -m) -R"
fi
reclaims() {
	best=
	for _ in 1 2 3 4 5 6 7 8 9 10; do
		n=$($SETARCH "$@" /usr/bin/time -v "$PROG" 2>&1 >/dev/null \
			| awk -F': ' '/Minor/ { print $2 }')
		if [ -z "$best" ] || [ "$n" -lt "$best" ]; then best=$n; fi
	done
	echo "$best"
}

# ---------------------------------------------------------------------------
title "Préliminaires"

for rule in all clean fclean re; do
	if make -n "$rule" >/dev/null 2>&1; then ok "règle make '$rule'"
	else ko "règle make '$rule' absente"; fi
done

if command -v norminette >/dev/null 2>&1; then
	NORM=$(norminette srcs includes tests 2>&1 | grep "Error")
	if [ -z "$NORM" ]; then ok "norminette"
	else
		ko "norminette : $(echo "$NORM" | grep -c "Error!") fichier(s) en erreur"
		echo "$NORM" | grep "Error:" | sort | uniq -c | sort -rn | head -5
	fi
else
	info "norminette non installée : à vérifier à la main"
fi

# ---------------------------------------------------------------------------
title "Compilation de la librairie (HOSTTYPE=Testing)"

HOSTTYPE=Testing make re >/dev/null 2>&1
if [ -f libft_malloc_Testing.so ] \
	&& [ "$(readlink libft_malloc.so)" = "libft_malloc_Testing.so" ]; then
	ok "libft_malloc_Testing.so + lien libft_malloc.so -> $(readlink libft_malloc.so)"
else
	ko "libft_malloc_Testing.so ou le lien symbolique est manquant"
	exit 1
fi

# ---------------------------------------------------------------------------
title "Fonctions exportées (nm)"

for fn in malloc free realloc show_alloc_mem; do
	if nm -D libft_malloc.so | grep -q " T $fn$"; then ok "T $fn"
	else ko "$fn n'est pas exportée"; fi
done
echo "Fonctions externes utilisées :"
nm -D libft_malloc.so | awk '$1 == "U" { print "    " $2 }'

# ---------------------------------------------------------------------------
title "Compilation des tests"

mkdir -p "$BIN"
for i in 0 1 2 3 4 5; do
	$CC -w -o "$BIN/test$i" "tests/test$i.c" || ko "test$i.c ne compile pas"
done
$CC -w -o "$BIN/test6" tests/test6.c -L. -lft_malloc || ko "test6.c ne compile pas"
for t in threads edge debug; do
	$CC $BONUS_FLAGS -o "$BIN/$t" "tests/bonus/$t.c" -L. -lft_malloc -lpthread \
		|| ko "bonus/$t.c ne compile pas"
done
ok "tests compilés dans $BIN/"

# ---------------------------------------------------------------------------
title "Malloc test - Baseline / Memory allocation (test0, test1)"

PROG=$BIN/test0; SYS0=$(reclaims env)
PROG=$BIN/test1; SYS1=$(reclaims env)
PROG=$BIN/test0; LIB0=$(reclaims ./tests/run.sh)
PROG=$BIN/test1; LIB1=$(reclaims ./tests/run.sh)
PROG=$BIN/test2; LIB2=$(reclaims ./tests/run.sh)

echo "                 test0   test1   test2"
printf "  glibc         %6s  %6s\n" "$SYS0" "$SYS1"
printf "  libft_malloc  %6s  %6s  %6s\n" "$LIB0" "$LIB1" "$LIB2"

PAGES=$((LIB1 - LIB0))
if   [ "$PAGES" -lt 255 ];  then NOTE="0 : mémoire allouée insuffisante"
elif [ "$PAGES" -le 272 ];  then NOTE="5 : overhead correct"
elif [ "$PAGES" -le 312 ];  then NOTE="4 : overhead important"
elif [ "$PAGES" -le 512 ];  then NOTE="3 : overhead très important"
elif [ "$PAGES" -le 1022 ]; then NOTE="2 : overhead trop important"
else                             NOTE="1 : au moins une page par allocation"
fi
info "test1 - test0 = $PAGES pages -> note $NOTE"

# ---------------------------------------------------------------------------
title "Tests of free (test2)"

if [ "$LIB2" -lt "$LIB1" ]; then ok "free fonctionne ($LIB2 < $LIB1)"
else ko "free ne rend pas la mémoire ($LIB2 >= $LIB1)"; fi
if [ $((LIB2 - LIB0)) -le 3 ]; then
	ok "qualité du free : test2 - test0 = $((LIB2 - LIB0)) page(s) (<= 3)"
else
	ko "qualité du free : test2 - test0 = $((LIB2 - LIB0)) pages (> 3)"
fi

# ---------------------------------------------------------------------------
title "Realloc test / Realloc test++ / Error management (test3, test4, test5)"

check_output() {
	out=$(./tests/run.sh "$BIN/$1" Bonjour Bonjour 2>&1)
	status=$?
	if [ $status -ne 0 ]; then ko "$1 a planté (code $status)"
	elif [ "$out" = "$2" ]; then ok "$1 affiche « $(echo "$out" | paste -sd ' ') »"
	else ko "$1 affiche « $out »"; fi
}
check_output test3 "$(printf 'Bonjour\nBonjour')"
check_output test4 "$(printf 'Bonjour\nBonjour')"
check_output test5 "Bonjour"

# ---------------------------------------------------------------------------
title "Show_alloc_mem test (test6)"

OUT=$(./tests/run.sh "$BIN/test6")
echo "$OUT" | sed 's/^/    /'
EXPECTED="1024 32768 1048576 16777216 134217728"
GOT=$(echo "$OUT" | awk '/bytes$/ && !/Total/ { print $(NF - 1) }' | sort -n | tr '\n' ' ')
if [ "$GOT" = "$EXPECTED " ]; then ok "les 5 allocations sont affichées"
else ko "allocations affichées : $GOT"; fi
if echo "$OUT" | grep -q "^SMALL : 0x" && echo "$OUT" | grep -q "^LARGE : 0x"; then
	ok "zones SMALL / LARGE présentes"
else
	ko "en-têtes de zone SMALL / LARGE manquants"
fi
if echo "$OUT" | grep -q "^Total : 152077312 bytes$"; then ok "Total : 152077312 bytes"
else ko "mauvais Total"; fi

# ---------------------------------------------------------------------------
title "Bonus - Accès concurrents"

for fn in malloc free realloc show_alloc_mem; do
	if grep -rqs "pthread_mutex_lock" "srcs/api/$fn.c" "srcs/show/$fn.c"; then
		ok "mutex dans $fn"
	else
		info "mutex de $fn : à vérifier à la main dans le code"
	fi
done
OUT=$(LD_LIBRARY_PATH=. "$BIN/threads" | tail -1)
if [ "$OUT" = "Total : 0 bytes" ]; then ok "8 threads x 100000 malloc/realloc/free : $OUT"
else ko "threads : $OUT"; fi

# ---------------------------------------------------------------------------
title "Bonus - Cas limites"

OUT=$(LD_LIBRARY_PATH=. "$BIN/edge")
STATUS=$?
echo "$OUT" | grep -E "^(OK|FAIL)" | sed 's/^/    /'
if [ $STATUS -eq 0 ] && [ "$(echo "$OUT" | tail -1)" = "Total : 0 bytes" ]; then
	ok "cas limites sans fuite"
else
	ko "cas limites (code $STATUS)"
fi

# ---------------------------------------------------------------------------
title "Bonus - Variables de debug et show_alloc_mem_ex"

OUT=$(LD_LIBRARY_PATH=. FT_MALLOC_CHECK=1 "$BIN/debug" 2>&1 >/dev/null)
if echo "$OUT" | grep -q "double free"; then ok "FT_MALLOC_CHECK=1 : $OUT"
else ko "FT_MALLOC_CHECK=1 ne signale pas le double free"; fi

(LD_LIBRARY_PATH=. FT_MALLOC_CHECK=2 "$BIN/debug" >/dev/null 2>&1) 2>/dev/null
if [ $? -eq 134 ]; then ok "FT_MALLOC_CHECK=2 : abort() (SIGABRT)"
else ko "FT_MALLOC_CHECK=2 n'appelle pas abort()"; fi

N=$(LD_LIBRARY_PATH=. FT_MALLOC_VERBOSE=1 "$BIN/debug" 2>&1 >/dev/null | grep -c "^\[ft_malloc\] ")
if [ "$N" -ge 7 ]; then ok "FT_MALLOC_VERBOSE=1 : $N opérations tracées"
else ko "FT_MALLOC_VERBOSE=1 : $N opérations tracées"; fi

if LD_LIBRARY_PATH=. FT_MALLOC_SCRIBBLE=1 "$BIN/debug" | grep -q "AA AA AA AA"; then
	ok "FT_MALLOC_SCRIBBLE=1 : mémoire allouée remplie de 0xAA"
else
	ko "FT_MALLOC_SCRIBBLE=1 : pas de 0xAA dans les blocs alloués"
fi

info "show_alloc_mem_ex (blocs libres, hexdump, historique) :"
LD_LIBRARY_PATH=. "$BIN/debug" | sed 's/^/    /'

# ---------------------------------------------------------------------------
title "Résultat"

rm -f libft_malloc_Testing.so
make >/dev/null 2>&1
if [ $FAILS -eq 0 ]; then printf "${GREEN}Tous les tests passent.${RESET}\n"
else printf "${RED}%d test(s) en échec.${RESET}\n" "$FAILS"; fi
exit $((FAILS != 0))
