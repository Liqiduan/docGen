all:doxygen hugo
	mv dist/public/index.html dist/public/index_code.html 
	mv dist/public/doc/index.html dist/public/index.html

hugo:
	hugo/bin/hugo

doxygen:
	doxygen doxygen.cfg

.PHONY:all hugo doxygen
