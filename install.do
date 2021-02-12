redo-ifchange all
PREFIX=/usr/local
BINDIR=$PREFIX/bin
DOCDIR=$PREFIX/share/doc/table
MANDIR=$PREFIX/share/man/man1
install -d $BINDIR $DOCDIR $MANDIR
install -m 0755 table $BINDIR
install -m 0644 table.pdf $DOCDIR
install -m 0644 table.1.gz $MANDIR

