#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unistd.h>
#include <string>
#include <algorithm>
#include <random>
#include <set>
#include <unordered_map>

class TablePages {
    private:
    std::unordered_map<int, int> marcosPagina;
    std::set<int> available;
    int capacity;
    
    public:
	TablePages(int numFrames) {
        capacity = numFrames;
		for (int i=1;i<=numFrames;i++)
		    available.insert(i);
	}
    // retorna verdadero si quita marco a lowPriorityVP
    bool getFrame(int virtualPageNeeded, int lowPriorityVP) {
        if (referencedVirtualPage(virtualPageNeeded))
			return false;

        bool g = false;
        if (isFull()) {
            int marco = marcosPagina[lowPriorityVP];
            available.insert(marco);
            marcosPagina.erase(lowPriorityVP);
            printf("Got physical page %d, from %d\n", marco, lowPriorityVP);
            g = true;
        }
		int marcoDisponible = *available.begin();
        printf("Gave physical page %d to virtual page %d\n", marcoDisponible, virtualPageNeeded);
		marcosPagina[virtualPageNeeded] = marcoDisponible;
        available.erase(marcoDisponible);
		return g;
	}
	bool referencedVirtualPage(int virtualPage) {
		return marcosPagina.count(virtualPage) > 0;
	}
    unsigned size() {
        return marcosPagina.size();
    }
    bool isFull() {
        return marcosPagina.size() == capacity;
    }
    void print() {
        printf("--> ");
        for (std::unordered_map<int, int>::iterator it = marcosPagina.begin(); it != marcosPagina.end(); it++)
            printf("%2d ", it->first);
        printf("\n--> ");
        for (std::unordered_map<int, int>::iterator it = marcosPagina.begin(); it != marcosPagina.end(); it++)
            printf("%2d ", it->second);
        printf("\n");
    }
};

void loadReferences(const std::string &filename, std::vector<int> &pageRefs)
{
    std::ifstream file(filename); // Abre el archivo en modo de lectura
    if (!file.is_open())
    {                                                                          // Verifica si el archivo no se abrió correctamente
        std::cerr << "Error al abrir el archivo de referencias." << std::endl; // Muestra un mensaje de error
        exit(1);                                                               // Termina el programa con un código de error
    }
    int page;
    while (file >> page)
    {                             // Lee cada número de página del archivo
        pageRefs.push_back(page); // Lo agrega al vector de referencias
    }
    file.close(); // Cierra el archivo después de leerlo
}

// Algoritmo FIFO
int simulateFIFO(const std::vector<int> &pageRefs, int numFrames) {
    std::queue<int> virtualPages; // Cola para mantener el orden de llegada de páginas virtuales
    TablePages *tp = new TablePages(numFrames);

    int pageFaults = 0;         // Contador de fallos de página

    for (int i = 0; i < pageRefs.size(); i++) {
        int page = pageRefs[i];
        // printf("%2d: %d \n", i, page);
        // tp->print();

        if (!tp->referencedVirtualPage(page)) {
            pageFaults++;
            
            if (tp->getFrame(page, virtualPages.front())) // el marco fue reasignado
                virtualPages.pop();

            virtualPages.push(page);
        }
        // else
        //     printf("HIT\n");
        
        // printf("\n");
    }
    return pageFaults; // Devuelve el total de fallos de página
}

// Algoritmo LRU
int simulateLRU(const std::vector<int> &pageRefs, int numFrames) {
    std::vector<int> frames;
    int pageFaults = 0;

    TablePages *tp = new TablePages(numFrames);

    int i = 0;
    for (int page : pageRefs) { 
        i++;
        std::vector<int>::iterator it = find(frames.begin(), frames.end(), page);

        if (!tp->referencedVirtualPage(page)) {
            pageFaults++;

            int lowprio = -1;
            if (!frames.empty())
                lowprio = frames[0];

            if (tp->getFrame(page, lowprio)) // el marco fue reasignado
                frames.erase(frames.begin());
            
            frames.push_back(page);
        }
        else {
            frames.erase(it);
            frames.push_back(page);
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

// Algoritmo Óptimo
int simulateOptimal(const std::vector<int> &pageRefs, int numFrames) {
    std::vector<int> frames;
    int pageFaults = 0;
    TablePages *tp = new TablePages(numFrames);

    for (int i = 0; i < pageRefs.size(); ++i) { // Recorre cada referencia de página
        int page = pageRefs[i];
        printf("%d: %d\n", i+1, page);

        if (!tp->referencedVirtualPage(page)) {
            printf("MISS\n");
            pageFaults++;

            int pv = -1;
            if (tp->isFull()) {
                int indexToReplace = 0;
                int farthest = find(pageRefs.begin() + i + 1, pageRefs.end(), frames[0]) - pageRefs.begin();
                for (int j = 1; j < frames.size(); ++j) {
                    int nextUse = find(pageRefs.begin() + i + 1, pageRefs.end(), frames[j]) - pageRefs.begin();
                    if (nextUse > farthest) { // Busca la página que se usará más tarde
                        farthest = nextUse;
                        indexToReplace = j;
                    }
                }
                pv = frames[indexToReplace];
            }
            else
                frames.push_back(page);

            tp->getFrame(pageRefs[i], pv);
        }
        else
            printf("HIT\n");

        // if (std::find(frames.begin(), frames.end(), page) == frames.end()){
        //     pageFaults++;
        //     if (frames.size() == numFrames) { // Si la memoria está llena
        //         int farthest = -1, indexToReplace = -1;
        //         for (int j = 0; j < frames.size(); ++j) {
        //             int nextUse = find(pageRefs.begin() + i + 1, pageRefs.end(), frames[j]) - pageRefs.begin();
        //             if (nextUse > farthest) { // Busca la página que se usará más tarde
        //                 farthest = nextUse;
        //                 indexToReplace = j;
        //             }
        //         }

        //         frames.erase(frames.begin() + indexToReplace); // Reemplaza esa página
        //     }
        //     frames.push_back(page); // Agrega la nueva página
        // }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

// Algoritmo Clock
int simulateClock(const std::vector<int> &pageRefs, int numFrames)
{
    std::vector<int> frames(numFrames, -1);     // Vector inicializado con -1 para representar marcos vacíos
    std::vector<bool> useBit(numFrames, false); // Bits de uso para cada marco
    int pageFaults = 0, pointer = 0;            // Contador de fallos y puntero del reloj

    for (int page : pageRefs)
    { // Recorre cada referencia de página
        auto it = find(frames.begin(), frames.end(), page);
        if (it == frames.end())
        {                 // Si no está en memoria
            pageFaults++; // Incrementa los fallos de página
            while (useBit[pointer])
            { // Busca un marco con bit de uso 0
                useBit[pointer] = false;
                pointer = (pointer + 1) % numFrames; // Avanza el puntero
            }
            frames[pointer] = page;              // Reemplaza la página
            useBit[pointer] = true;              // Marca el bit de uso como 1
            pointer = (pointer + 1) % numFrames; // Avanza el puntero
        }
        else
        {                                       // Si está en memoria
            useBit[it - frames.begin()] = true; // Marca el bit de uso como 1
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

int main(int argc, char *argv[]) {
    int numFrames = 0;
    std::string algorithm;
    std::string filename;

    int opt;
    while ((opt = getopt(argc, argv, "m:a:f:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            try 
            {
                numFrames = std::stoi(optarg);
            } 
            catch (const std::invalid_argument& e) 
            {
                std::cerr << "El tamaño de marco no es un entero" << std::endl;
                return 1;
            }    
            break;
        case 'a':
            algorithm = optarg;
            break;
        case 'f':
            filename = optarg;
            break;
        default:
            std::cerr << argv[0] << " -m <num_frames> -a <algoritmo> -f <archivo_referencias>" << std::endl;
            return 1;
        }
    }
    
    if (numFrames < 1) 
    {
        std::cerr << "Debe ingresar número de marcos válido." << std::endl;
        return 1;
    }
    
    std::vector<int> pageRefs;
    loadReferences(filename, pageRefs); // Carga las referencias de página

    int pageFaults = 0;
    if (algorithm == "FIFO")
    {
        pageFaults = simulateFIFO(pageRefs, numFrames);
    }
    else if (algorithm == "LRU")
    {
        pageFaults = simulateLRU(pageRefs, numFrames);
    }
    else if (algorithm == "Optimal")
    {
        pageFaults = simulateOptimal(pageRefs, numFrames);
    }
    else if (algorithm == "Clock")
    {
        pageFaults = simulateClock(pageRefs, numFrames);
    }
    else
    {
        std::cout << "Algoritmo no reconocido. Use FIFO, LRU, Optimal o Clock." << std::endl;
        return 1;
    }

<<<<<<< HEAD
    std::cout << pageRefs.size()-pageFaults << " - " << pageFaults << std::endl;
    std::cout << "Fallos de página: " << pageFaults << std::endl; // Imprime los fallos de página
=======
    std::cout << "Fallos de pagina: " << pageFaults << std::endl; // Imprime los fallos de página
>>>>>>> main
    return 0;
}
