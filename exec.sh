# COMPILAZIONE
# Il comando 'make' necessita del makefile, che deve essere
# creato come descritto nella guida sulla pagina Elearn
  make clean
  clear
  make
  read -p "Compilazione eseguita. Premi invio per eseguire..."
# Esecuzioe del DS sulla porta 4242
  gnome-terminal -x sh -c "./discoveryserver 4242; exec bash"
# Esecuzione di 5 peer sulle porte {5001,...,5005}
  for port in {5001..5005}
  do
     gnome-terminal -x sh -c "./peer $port; exec bash"
  done
