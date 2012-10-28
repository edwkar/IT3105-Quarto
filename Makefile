OBJS = quarto_agent.o
CC = g++
DEBUG = -g
CFLAGS = -std=c++0x -O2 -Wall -Wextra -c $(DEBUG)
LFLAGS = -Wall -Wextra $(DEBUG)
TEX_COMPILER = pdflatex

all: quarto_agent report

quarto_agent: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o quarto_agent

quarto_agent.o: quarto_agent.cc
	$(CC) $(CFLAGS) quarto_agent.cc

report: *.tex
	$(TEX_COMPILER) quarto-lekvam-karlsen.tex &&\
	$(TEX_COMPILER) quarto-lekvam-karlsen.tex &&\
  $(TEX_COMPILER) quarto-lekvam-karlsen.tex

clean: 
	rm -f quarto_agent 
	rm -f quarto-lekvam-karlsen.pdf
	rm -f *~
	find . | egrep "\.(o|out|log|bbl|aux)$$" | xargs rm -f
