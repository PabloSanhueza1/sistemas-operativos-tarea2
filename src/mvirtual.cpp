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
            // printf("Got physical page %d, from %d\n", marco, lowPriorityVP);
            g = true;
        }
		int marcoDisponible = *available.begin();
        // printf("Gave physical page %d to virtual page %d\n", marcoDisponible, virtualPageNeeded);
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

void loadReferences(const std::string &filename, std::vector<int> &pageRefs) {
    std::ifstream file(filename); // Abre el archivo en modo de lectura
    if (!file.is_open()) { 
        std::cerr << "Error al abrir el archivo de referencias." << std::endl;
        exit(1);
    }
    int page;
    while (file >> page) {
        pageRefs.push_back(page); // Lo agrega al vector de referencias
    }
    file.close(); // Cierra el archivo después de leerlo
}

int simulateFIFO(const std::vector<int> &pageRefs, int numFrames) {
    std::queue<int> virtualPages; // Cola para mantener el orden de llegada de páginas virtuales
    TablePages *tp = new TablePages(numFrames);

    int pageFaults = 0;         // Contador de fallos de página

    for (int i = 0; i < pageRefs.size(); i++) {
        int page = pageRefs[i];

        if (!tp->referencedVirtualPage(page)) {
            pageFaults++;

            // el marco fue reasignado
            if (tp->getFrame(page, virtualPages.front()))
                virtualPages.pop();

            virtualPages.push(page);
        }
    }
    return pageFaults; // Devuelve el total de fallos de página
}

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
    return pageFaults;
}

// retorna indice
int farthest(const std::vector<int> &pageRefs, const std::vector<int> &frames, int from) {
    if (frames.empty())
        return -1;
    int indexToReplace = 0;
    int mx = find(pageRefs.begin() + from + 1, pageRefs.end(), frames[0]) - pageRefs.end();
    int distance;
    for (int i = 1; i < frames.size(); i++) {
        distance = find(pageRefs.begin() + from + 1, pageRefs.end(), frames[i]) - pageRefs.end();
        if (distance > mx) {
            mx = distance;
            indexToReplace = i;
        }
    }
    return indexToReplace;
}

int simulateOptimal(const std::vector<int> &pageRefs, int numFrames) {
    std::vector<int> frames;
    int pageFaults = 0;
    TablePages *tp = new TablePages(numFrames);

    for (int i = 0; i < pageRefs.size(); ++i) {
        int page = pageRefs[i];

        if (!tp->referencedVirtualPage(page)) {
            pageFaults++;

            int lowprio = -1;
            int indexToReplace = farthest(pageRefs, frames, i);
            if (tp->isFull())
                lowprio = frames[indexToReplace];

            if (tp->getFrame(pageRefs[i], lowprio))
                frames.erase(frames.begin() + indexToReplace);
            
            frames.push_back(page);
        }
    }
    return pageFaults;
}

// Algoritmo Clock
int simulateClock(const std::vector<int> &pageRefs, int numFrames) {
    std::vector<int> frames;
    std::vector<bool> useBit;
    int pageFaults = 1, pointer = 0;            
    TablePages *tp = new TablePages(numFrames);

    frames.push_back(pageRefs[0]);
    useBit.push_back(true);
    tp->getFrame(pageRefs[0], -1);

    for (int i = 1; i < pageRefs.size(); i++) {
        int page = pageRefs[i];

        if (!tp->referencedVirtualPage(page)) {
            pageFaults++; 
            if (!tp->isFull()) {
                tp->getFrame(page, -1);
                frames.push_back(page);
                useBit.push_back(true);
            }
            else {
                while (useBit[pointer]) {
                    useBit[pointer] = false;
                    pointer = (pointer + 1) % frames.size();
                }
                tp->getFrame(page, frames[pointer]);
                frames[pointer] = page;
                useBit[pointer] = true;
                pointer = (pointer + 1) % numFrames; 
            }
        }
        else {
            std::vector<int>::iterator it = frames.begin();
            useBit[it - frames.begin()] = true;
        }
    }
    return pageFaults;
}

int main(int argc, char *argv[]) {
    int numFrames = 0;
    std::string algorithm;
    std::string filename;

    int opt;
    while ((opt = getopt(argc, argv, "m:a:f:")) != -1) {
        switch (opt) {
            case 'm':
                try {
                    numFrames = std::stoi(optarg);
                } 
                catch (const std::invalid_argument& e) {
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
    
    if (numFrames < 1) {
        std::cerr << "Debe ingresar número de marcos válido." << std::endl;
        return 1;
    }
    
    std::vector<int> pageRefs;
    loadReferences(filename, pageRefs); // Carga las referencias de página

    int pageFaults = 0;

    if (algorithm == "FIFO")
        pageFaults = simulateFIFO(pageRefs, numFrames);

    else if (algorithm == "LRU")
        pageFaults = simulateLRU(pageRefs, numFrames);

    else if (algorithm == "Optimal")
        pageFaults = simulateOptimal(pageRefs, numFrames);

    else if (algorithm == "Clock")
        pageFaults = simulateClock(pageRefs, numFrames);

    else {
        std::cout << "Algoritmo no reconocido. Use FIFO, LRU, Optimal o Clock." << std::endl;
        return 1;
    }

    // std::cout << pageRefs.size()-pageFaults << " - " << pageFaults << std::endl;
    std::cout << "Fallos de página: " << pageFaults << std::endl; // Imprime los fallos de página
    return 0;
}
