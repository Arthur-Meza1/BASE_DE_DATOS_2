#include <iostream>
#include <fstream>
#include <sys/stat.h> // Para mkdir
#include <cstdio>
#include <cstring>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <string>
#include <sstream>
#include <numeric>
#include <limits> // Para numeric_limits

#ifdef _WIN32
#include <direct.h> 
#define MKDIR(path) _mkdir(path)
#else
#include <unistd.h> // Para mkdir en sistemas Unix/Linux
#define MKDIR(path) mkdir(path, 0777) // 0777 para permisos rwx para todos
#endif

using namespace std;

// Estructura para almacenar los metadatos de un registro
struct RecordMetadata {
    long idRegistro;
    int platoIdx;
    int superficieIdx;
    int pistaIdx; // Índice de pista global en la superficie
    int sectorGlobalEnPista; // Índice de sector dentro de la pista global
    long offset; // Offset dentro del sector
    int tamRegistro; // Tamaño real del registro (incluyendo '\n')
    bool ocupado; // True si el registro está ocupado, false si está "eliminado"
};

// Clase para un Sector en el disco
class Sector {
private:
    string rutaArchivo;
    int capacidadBytes; // Capacidad máxima en bytes del sector

public:
    // Constructor: Crea o abre el archivo del sector.
    Sector(const string& ruta, int capacidad) : rutaArchivo(ruta), capacidadBytes(capacidad) {
        // Este constructor solo inicializa los miembros.
    }

    // Obtener el tamaño actual del archivo del sector
    long obtenerTamArchivo() {
        FILE* f = fopen(rutaArchivo.c_str(), "rb");
        if (!f) return 0; // Archivo vacío o no existe aún
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fclose(f);
        return size;
    }

    // Escribir datos en el sector (modo append). NO AÑADE SALTO DE LÍNEA.
    bool escribir(const string& datos) {
        ofstream archivo(rutaArchivo, ios::app);
        if (!archivo.is_open()) {
            cerr << "Error: No se pudo abrir el sector para escribir: " << rutaArchivo << endl;
            return false;
        }
        archivo << datos; // Escribe los datos exactamente como vienen
        archivo.close();
        return true;
    }

    // Sobrecarga de escribir para sobrescribir el contenido (útil para el diccionario)
    bool escribir(const string& datos, bool sobrescribir) {
        if (sobrescribir) {
            ofstream archivo(rutaArchivo, ios::trunc); // Abrir en modo truncar para sobrescribir
            if (!archivo.is_open()) {
                cerr << "Error: No se pudo abrir el sector para sobrescribir: " << rutaArchivo << endl;
                return false;
            }
            archivo << datos;
            archivo.close();
            return true;
        } else {
            return escribir(datos); // Llama a la versión de append
        }
    }

    // Leer todo el contenido del sector
    string leerTodo() {
        ifstream archivo(rutaArchivo);
        if (!archivo.is_open()) {
            return ""; // O lanza una excepción, según el manejo de errores deseado
        }
        stringstream buffer;
        buffer << archivo.rdbuf();
        return buffer.str();
    }

    // Leer una parte específica del sector
    string leer(long offset, int tamano) {
        ifstream archivo(rutaArchivo);
        if (!archivo.is_open()) {
            return "";
        }
        archivo.seekg(offset);
        char* buffer = new char[tamano + 1];
        archivo.read(buffer, tamano);
        buffer[tamano] = '\0'; // Asegurar terminación nula
        string resultado(buffer);
        delete[] buffer;
        return resultado;
    }

    // Obtener la capacidad máxima del sector
    int getCapacidadBytes() const {
        return capacidadBytes;
    }

    string getRutaArchivo() const {
        return rutaArchivo;
    }

    // Eliminar contenido del sector (lógicamente, marcando registros como no ocupados)
    // Para una eliminación física, se necesitaría reescribir el sector.
    // En esta simulación, solo se vacía el archivo del sector.
    bool vaciarSector() {
        ofstream archivo(rutaArchivo, ios::trunc); // Abre y trunca el archivo
        if (!archivo.is_open()) {
            cerr << "Error: No se pudo vaciar el sector: " << rutaArchivo << endl;
            return false;
        }
        archivo.close();
        return true;
    }
};

// Clase para una Pista en el disco
class Pista {
private:
    vector<Sector*> sectores;
    int numSectores;
    int capacidadSectorBytes;

public:
    Pista(const string& rutaBase, int idPlato, int idSuperficie, int idPista, int nSectores, int capSector)
        : numSectores(nSectores), capacidadSectorBytes(capSector) {
        // Crear los sectores para esta pista
        for (int i = 0; i < numSectores; ++i) {
            string rutaSector = rutaBase + "/P" + to_string(idPlato) + "/S" + to_string(idSuperficie) + "/Track" + to_string(idPista) + "/Sector" + to_string(i) + ".txt";
            sectores.push_back(new Sector(rutaSector, capacidadSectorBytes));
        }
    }

    ~Pista() {
        for (Sector* s : sectores) {
            delete s;
        }
    }

    Sector* getSector(int idx) {
        if (idx >= 0 && idx < numSectores) {
            return sectores[idx];
        }
        return nullptr;
    }

    int getNumSectores() const {
        return numSectores;
    }
};

// Clase para una Superficie en el disco
class Superficie {
private:
    vector<Pista*> pistas;
    int numPistas;
    int numSectoresPorPista;
    int capacidadSectorBytes;

public:
    Superficie(const string& rutaBase, int idPlato, int idSuperficie, int nPistas, int nSectoresPorPista, int capSector)
        : numPistas(nPistas), numSectoresPorPista(nSectoresPorPista), capacidadSectorBytes(capSector) {
        // Crear las pistas para esta superficie
        for (int i = 0; i < numPistas; ++i) {
            string rutaPista = rutaBase + "/P" + to_string(idPlato) + "/S" + to_string(idSuperficie) + "/Track" + to_string(i);
            MKDIR(rutaPista.c_str()); // Crear directorio para la pista
            pistas.push_back(new Pista(rutaBase, idPlato, idSuperficie, i, numSectoresPorPista, capacidadSectorBytes));
        }
    }

    ~Superficie() {
        for (Pista* p : pistas) {
            delete p;
        }
    }

    Pista* getPista(int idx) {
        if (idx >= 0 && idx < numPistas) {
            return pistas[idx];
        }
        return nullptr;
    }

    int getNumPistas() const {
        return numPistas;
    }
};

// Clase para un Plato en el disco
class Plato {
private:
    vector<Superficie*> superficies;
    int numSuperficies;
    int numPistasPorSuperficie;
    int numSectoresPorPista;
    int capacidadSectorBytes;

public:
    Plato(const string& rutaBase, int idPlato, int nSuperficies, int nPistasPorSuperficie, int nSectoresPorPista, int capSector)
        : numSuperficies(nSuperficies), numPistasPorSuperficie(nPistasPorSuperficie), numSectoresPorPista(nSectoresPorPista), capacidadSectorBytes(capSector) {
        // Crear las superficies para este plato
        for (int i = 0; i < numSuperficies; ++i) {
            string rutaSuperficie = rutaBase + "/P" + to_string(idPlato) + "/S" + to_string(i);
            MKDIR(rutaSuperficie.c_str()); // Crear directorio para la superficie
            superficies.push_back(new Superficie(rutaBase, idPlato, i, numPistasPorSuperficie, numSectoresPorPista, capacidadSectorBytes));
        }
    }

    ~Plato() {
        for (Superficie* s : superficies) {
            delete s;
        }
    }

    Superficie* getSuperficie(int idx) {
        if (idx >= 0 && idx < numSuperficies) {
            return superficies[idx];
        }
        return nullptr;
    }

    int getNumSuperficies() const {
        return numSuperficies;
    }
};

// Clase principal para el Disco
class Disco {
private:
    string nombreDisco;
    int numPlatos;
    int numSuperficiesPorPlato;
    int numPistasPorSuperficie;
    int numSectoresPorPista;
    int capacidadSectorBytes;
    vector<Plato*> platos;
    string rutaBaseDisco; // Ruta base donde se guardará el disco

    string tablaEsquema; // Esquema de la tabla, ej: "id#nombre#edad"
    vector<RecordMetadata> diccionarioDeDatosEnRAM; // Diccionario de datos en RAM

    // Propiedades de la última posición escrita para optimizar la búsqueda secuencial
    int lastPlatoWritten;
    int lastSuperficieWritten;
    int lastPistaWritten;
    int lastSectorWritten;

    // Función auxiliar para verificar si un sector es reservado (Sector0.txt o Sector1.txt)
    bool isReservedSector(int platoIdx, int superficieIdx, int pistaIdx, int sectorIdx) {
        return (platoIdx == 0 && superficieIdx == 0 && pistaIdx == 0 && (sectorIdx == 0 || sectorIdx == 1));
    }

    // Carga el diccionario de datos desde el disco a la RAM
    void cargarDiccionario() {
        string rutaSector1 = rutaBaseDisco + "/P0/S0/Track0/Sector1.txt";
        Sector sector1(rutaSector1, capacidadSectorBytes); // Usar el sector real

        string contenido = sector1.leerTodo();
        stringstream ss(contenido);
        string linea;

        diccionarioDeDatosEnRAM.clear(); // Limpiar el diccionario actual

        // Leer la línea de configuración (primera línea del Sector1.txt)
        getline(ss, linea); // Ignorar la primera línea de configuración

        while (getline(ss, linea)) {
            if (linea.empty()) continue; // Salta líneas vacías
            stringstream ss_linea(linea);
            string segmento;
            vector<string> segmentos;

            while (getline(ss_linea, segmento, '#')) {
                segmentos.push_back(segmento);
            }

            // Asegurarse de que hay suficientes segmentos para un RecordMetadata completo
            if (segmentos.size() >= 8) {
                RecordMetadata rm;
                // Asumiendo el formato "R#id#plato#superficie#pista#sector#offset#tam#ocupado"
                rm.idRegistro = stol(segmentos[1]);
                rm.platoIdx = stoi(segmentos[2]);
                rm.superficieIdx = stoi(segmentos[3]);
                rm.pistaIdx = stoi(segmentos[4]);
                rm.sectorGlobalEnPista = stoi(segmentos[5]);
                rm.offset = stol(segmentos[6]);
                rm.tamRegistro = stoi(segmentos[7]);
                rm.ocupado = (segmentos[8] == "1"); // Convertir "1" a true, "0" a false

                diccionarioDeDatosEnRAM.push_back(rm);
            }
        }
    }

    // Persiste el diccionario de datos de la RAM al disco (Sector1.txt)
    void persistirDiccionario() {
        string rutaSector1 = rutaBaseDisco + "/P0/S0/Track0/Sector1.txt";
        Sector sector1(rutaSector1, capacidadSectorBytes); // Usar el sector real

        stringstream ss;
        // Escribir la configuración del disco en la primera línea
        ss << "CONFIG#" << numPlatos << "#" << numSuperficiesPorPlato << "#"
           << numPistasPorSuperficie << "#" << numSectoresPorPista << "#"
           << capacidadSectorBytes << "#" << nombreDisco << "\n";

        for (const auto& rm : diccionarioDeDatosEnRAM) {
            // Solo persiste los registros ocupados
            if (rm.ocupado) {
                ss << "R#" << rm.idRegistro << "#" << rm.platoIdx << "#"
                   << rm.superficieIdx << "#" << rm.pistaIdx << "#"
                   << rm.sectorGlobalEnPista << "#" << rm.offset << "#"
                   << rm.tamRegistro << "#" << (rm.ocupado ? "1" : "0") << "\n";
            }
        }
        sector1.escribir(ss.str(), true); // Sobrescribir el contenido del Sector1.txt
    }

    // Extrae el esquema de tabla del Sector0.txt
    void cargarEsquema() {
        string rutaSector0 = rutaBaseDisco + "/P0/S0/Track0/Sector0.txt";
        Sector sector0(rutaSector0, capacidadSectorBytes);

        string contenido = sector0.leerTodo();
        if (!contenido.empty()) {
            size_t pos = contenido.find("R1#");
            if (pos != string::npos) {
                tablaEsquema = contenido.substr(pos + 3); // Extraer después de "R1#"
                // Eliminar el salto de línea final si existe
                if (!tablaEsquema.empty() && tablaEsquema.back() == '\n') {
                    tablaEsquema.pop_back();
                }
            } else {
                tablaEsquema = "";
            }
        } else {
            tablaEsquema = "";
        }
    }

    // Transforma un archivo CSV a un stringstream con '#' como delimitador
    stringstream transformarCSV_a_stringstream(const string& rutaCSV) {
        ifstream archivoCSV(rutaCSV);
        stringstream ssSalida;
        if (!archivoCSV.is_open()) {
            cerr << "Error: No se pudo abrir el archivo CSV: " << rutaCSV << endl;
            return ssSalida;
        }

        string linea;
        while (getline(archivoCSV, linea)) {
            // Reemplazar comas con '#'
            for (char &c : linea) {
                if (c == ',') {
                    c = '#';
                }
            }
            ssSalida << linea << "\n";
        }
        archivoCSV.close();
        return ssSalida;
    }

    // Calcula el próximo ID de registro disponible
    long getNextRecordId() {
        if (diccionarioDeDatosEnRAM.empty()) {
            return 1;
        }
        long maxId = 0;
        for (const auto& rm : diccionarioDeDatosEnRAM) {
            if (rm.idRegistro > maxId) {
                maxId = rm.idRegistro;
            }
        }
        return maxId + 1;
    }

    // Nuevo método para encontrar el espacio disponible siguiendo la lógica cilíndrica
    tuple<int, int, int, int, long> encontrarEspacioCilindrico(int tamanoRequerido) {
        // Intentar continuar desde la última posición escrita para locality
        int startPlato = lastPlatoWritten;
        int startPista = lastPistaWritten;
        int startSuperficie = lastSuperficieWritten;
        int startSector = lastSectorWritten;

        // Bucle principal para recorrer los platos
        for (int p = 0; p < numPlatos; ++p) {
            // Asegurarse de que la iteración comience desde el plato correcto
            int current_plato = (startPlato + p) % numPlatos;

            // Bucle interno para recorrer las pistas dentro de cada plato (cilindro)
            for (int t = 0; t < numPistasPorSuperficie; ++t) {
                // Asegurarse de que la iteración comience desde la pista correcta
                int current_pista = (startPista + t) % numPistasPorSuperficie;

                // Bucle para recorrer las superficies del plato actual, manteniendo la misma pista (cilindro)
                for (int s = 0; s < numSuperficiesPorPlato; ++s) {
                    // Asegurarse de que la iteración comience desde la superficie correcta
                    int current_superficie = (startSuperficie + s) % numSuperficiesPorPlato;

                    // Si ya hemos pasado por todas las superficies para la pista actual,
                    // y aún no hemos encontrado espacio, empezar a buscar desde el sector 0
                    // en la siguiente iteración de superficie.
                    // Esto evita saltarse sectores dentro de una pista.
                    int actual_start_sector = (t == 0 && s == 0) ? startSector : 0;


                    Pista* pistaObj = platos[current_plato]->getSuperficie(current_superficie)->getPista(current_pista);
                    if (pistaObj == nullptr) continue;

                    for (int sec = 0; sec < numSectoresPorPista; ++sec) {
                        int current_sector = (actual_start_sector + sec) % numSectoresPorPista;

                        if (isReservedSector(current_plato, current_superficie, current_pista, current_sector)) {
                            continue; // Ignorar sectores reservados
                        }

                        Sector* sectorObj = pistaObj->getSector(current_sector);
                        if (sectorObj == nullptr) continue;

                        long tamActual = sectorObj->obtenerTamArchivo();
                        if (tamActual + tamanoRequerido <= sectorObj->getCapacidadBytes()) {
                            // Espacio encontrado. Actualizar la última posición escrita.
                            lastPlatoWritten = current_plato;
                            lastSuperficieWritten = current_superficie;
                            lastPistaWritten = current_pista;
                            lastSectorWritten = current_sector;
                            return make_tuple(current_plato, current_superficie, current_pista, current_sector, tamActual);
                        }
                    }
                }
            }
        }
        return make_tuple(-1, -1, -1, -1, -1); // No hay espacio
    }


public:
    Disco(int nPlatos, int nSuperficies, int nPistas, int nSectores, int capSector, const string& nombre)
        : numPlatos(nPlatos), numSuperficiesPorPlato(nSuperficies), numPistasPorSuperficie(nPistas),
          numSectoresPorPista(nSectores), capacidadSectorBytes(capSector), nombreDisco(nombre),
          lastPlatoWritten(0), lastSuperficieWritten(0), lastPistaWritten(0), lastSectorWritten(0) {
        rutaBaseDisco = "./" + nombreDisco + "_disk";
        MKDIR(rutaBaseDisco.c_str()); // Crear directorio base del disco

        // Crear la estructura física del disco
        for (int i = 0; i < numPlatos; ++i) {
            string rutaPlato = rutaBaseDisco + "/P" + to_string(i);
            MKDIR(rutaPlato.c_str()); // Crear directorio para el plato
            platos.push_back(new Plato(rutaBaseDisco, i, numSuperficiesPorPlato, numPistasPorSuperficie, numSectoresPorPista, capacidadSectorBytes));
        }

        // Inicializar Sector0.txt para el esquema y Sector1.txt para el diccionario
        // Estos deben existir siempre, incluso si están vacíos al inicio.
        Sector sector0_init(rutaBaseDisco + "/P0/S0/Track0/Sector0.txt", capacidadSectorBytes);
        Sector sector1_init(rutaBaseDisco + "/P0/S0/Track0/Sector1.txt", capacidadSectorBytes);

        // Si es un disco nuevo, inicializar el diccionario vacío y el esquema vacío
        persistirDiccionario(); // Escribe la configuración inicial del disco
        cargarEsquema(); // Carga esquema (inicialmente vacío)
    }

    ~Disco() {
        for (Plato* p : platos) {
            delete p;
        }
    }

    // Cargar un disco existente desde su ruta base
    static Disco* cargarDisco(const string& ruta) {
        ifstream config_file(ruta + "/P0/S0/Track0/Sector1.txt");
        if (!config_file.is_open()) {
            cerr << "Error: No se pudo cargar la configuración del disco desde " << ruta << endl;
            return nullptr;
        }

        string linea_config;
        getline(config_file, linea_config);
        config_file.close();

        stringstream ss_config(linea_config);
        string segmento;
        vector<string> segmentos_config;

        while (getline(ss_config, segmento, '#')) {
            segmentos_config.push_back(segmento);
        }

        if (segmentos_config.size() < 7 || segmentos_config[0] != "CONFIG") {
            cerr << "Error: Formato de configuración de disco inválido." << endl;
            return nullptr;
        }

        int nPlatos = stoi(segmentos_config[1]);
        int nSuperficies = stoi(segmentos_config[2]);
        int nPistas = stoi(segmentos_config[3]);
        int nSectores = stoi(segmentos_config[4]);
        int capSector = stoi(segmentos_config[5]);
        string nombre = segmentos_config[6];

        Disco* disco = new Disco(nPlatos, nSuperficies, nPistas, nSectores, capSector, nombre);
        disco->rutaBaseDisco = ruta; // Asegurar que la ruta base es la correcta
        disco->cargarDiccionario(); // Cargar diccionario de datos
        disco->cargarEsquema(); // Cargar esquema
        cout << "Disco '" << nombre << "' cargado exitosamente desde " << ruta << endl;
        return disco;
    }

    // Métodos públicos para interactuar con el disco

    // Carga un archivo CSV y lo almacena en el disco
    void cargarCSV(const string& rutaCSV) {
        stringstream ssCSV = transformarCSV_a_stringstream(rutaCSV);
        if (ssCSV.str().empty()) {
            cerr << "El archivo CSV está vacío o no se pudo procesar." << endl;
            return;
        }

        string linea;
        // La primera línea es el esquema
        getline(ssCSV, linea);
        if (linea.empty()) {
            cerr << "El archivo CSV no tiene esquema." << endl;
            return;
        }

        // Almacenar el esquema en Sector0.txt
        string rutaSector0 = rutaBaseDisco + "/P0/S0/Track0/Sector0.txt";
        Sector sector0(rutaSector0, capacidadSectorBytes);
        string esquemaConPrefijo = "R1#" + linea + "\n";
        sector0.escribir(esquemaConPrefijo, true); // Sobreescribir esquema
        tablaEsquema = linea; // Actualizar esquema en RAM

        cout << "Esquema cargado: " << tablaEsquema << endl;

        // Leer y almacenar los registros
        while (getline(ssCSV, linea)) {
            if (!linea.empty()) {
                insertarRegistro(linea); // Reutilizar la función de inserción
            }
        }
        cout << "Datos del CSV cargados y persistidos." << endl;
    }

    // Inserta un nuevo registro en el disco
    void insertarRegistro(const string& datosRegistro) {
        // Calcular el tamaño del registro (datos + delimitador de nueva línea)
        int tamanoRequerido = datosRegistro.length() + 1; // +1 para el '\n'

        // Encontrar espacio en el disco utilizando la lógica cilíndrica
        auto [platoIdx, superficieIdx, pistaIdx, sectorGlobalEnPista, offset] = encontrarEspacioCilindrico(tamanoRequerido);

        if (platoIdx == -1) {
            cout << "No hay espacio suficiente en el disco para el registro: " << datosRegistro << endl;
            return;
        }

        // Obtener el sector donde se escribirá el registro
        Sector* sectorAEscribir = platos[platoIdx]
                                   ->getSuperficie(superficieIdx)
                                   ->getPista(pistaIdx)
                                   ->getSector(sectorGlobalEnPista);

        if (sectorAEscribir == nullptr) {
            cerr << "Error: Sector no encontrado en la ubicación calculada." << endl;
            return;
        }

        // Escribir el registro en el sector
        string registroConSalto = datosRegistro + "\n";
        if (!sectorAEscribir->escribir(registroConSalto)) {
            cerr << "Error al escribir el registro en el sector: " << sectorAEscribir->getRutaArchivo() << endl;
            return;
        }

        // Actualizar el diccionario de datos en RAM
        RecordMetadata nuevoRM;
        nuevoRM.idRegistro = getNextRecordId();
        nuevoRM.platoIdx = platoIdx;
        nuevoRM.superficieIdx = superficieIdx;
        nuevoRM.pistaIdx = pistaIdx;
        nuevoRM.sectorGlobalEnPista = sectorGlobalEnPista;
        nuevoRM.offset = offset;
        nuevoRM.tamRegistro = tamanoRequerido;
        nuevoRM.ocupado = true;
        diccionarioDeDatosEnRAM.push_back(nuevoRM);

        cout << "Registro ID " << nuevoRM.idRegistro << " insertado en P" << platoIdx << "/S" << superficieIdx
             << "/T" << pistaIdx << "/Sec" << sectorGlobalEnPista << " @offset " << offset << endl;

        // Persistir el diccionario actualizado al disco
        persistirDiccionario();
    }

    // Recupera un registro por su ID
    string recuperarRegistro(long id) {
        for (const auto& rm : diccionarioDeDatosEnRAM) {
            if (rm.idRegistro == id && rm.ocupado) {
                Sector* sector = platos[rm.platoIdx]
                                   ->getSuperficie(rm.superficieIdx)
                                   ->getPista(rm.pistaIdx)
                                   ->getSector(rm.sectorGlobalEnPista);
                if (sector) {
                    string registro = sector->leer(rm.offset, rm.tamRegistro);
                    // Eliminar el salto de línea al final si existe
                    if (!registro.empty() && registro.back() == '\n') {
                        registro.pop_back();
                    }
                    return registro;
                }
            }
        }
        return ""; // Registro no encontrado o eliminado
    }

    // Elimina un registro por su ID (marcando como no ocupado)
    void eliminarRegistro(long id) {
        bool encontrado = false;
        for (auto& rm : diccionarioDeDatosEnRAM) {
            if (rm.idRegistro == id) {
                if (rm.ocupado) {
                    rm.ocupado = false; // Marcamos como no ocupado
                    encontrado = true;
                    cout << "Registro ID " << id << " marcado como eliminado (lógicamente)." << endl;
                } else {
                    cout << "Registro ID " << id << " ya está eliminado." << endl;
                }
                break;
            }
        }
        if (!encontrado) {
            cout << "Registro ID " << id << " no encontrado." << endl;
        }
        persistirDiccionario(); // Persistir el cambio
    }

    // Muestra el mapa de bits de sectores ocupados/libres (simplificado)
    void mostrarMapaDeBits() {
        cout << "\n--- Mapa de Asignación de Sectores ---\n";
        for (int p = 0; p < numPlatos; ++p) {
            cout << "Plato " << p << ":\n";
            for (int s = 0; s < numSuperficiesPorPlato; ++s) {
                cout << "  Superficie " << s << ":\n";
                for (int t = 0; t < numPistasPorSuperficie; ++t) {
                    cout << "    Pista " << t << ": ";
                    Pista* pistaObj = platos[p]->getSuperficie(s)->getPista(t);
                    if (pistaObj) {
                        for (int sec = 0; sec < numSectoresPorPista; ++sec) {
                            if (isReservedSector(p, s, t, sec)) {
                                cout << "R"; // Sector reservado
                                continue;
                            }
                            Sector* sectorObj = pistaObj->getSector(sec);
                            if (sectorObj) {
                                if (sectorObj->obtenerTamArchivo() < sectorObj->getCapacidadBytes()) {
                                    bool tieneEspacio = false;
                                    for(const auto& rm : diccionarioDeDatosEnRAM) {
                                        if(rm.platoIdx == p && rm.superficieIdx == s && rm.pistaIdx == t && rm.sectorGlobalEnPista == sec && rm.ocupado) {
                                            tieneEspacio = true; // Si hay al menos un registro ocupado, se considera con datos
                                            break;
                                        }
                                    }
                                    if(tieneEspacio) {
                                         cout << "O"; // Ocupado (tiene algún registro)
                                    } else {
                                        cout << "L"; // Libre (no tiene registros o está vacío)
                                    }

                                } else {
                                    cout << "F"; // Lleno
                                }
                            } else {
                                cout << "?"; // Error
                            }
                        }
                    }
                    cout << "\n";
                }
            }
        }
        cout << "Leyenda: O=Ocupado (con registros), L=Libre (vacío o con espacio), F=Lleno, R=Reservado\n";
        cout << "-------------------------------------\n";
    }


    void mostrarEstadoDiccionario() {
        if (diccionarioDeDatosEnRAM.empty()) {
            cout << "Diccionario de datos en RAM está vacío.\n";
            return;
        }
        cout << "\n--- Estado del Diccionario de Datos en RAM ---\n";
        cout << setw(5) << "ID" << setw(8) << "Plato" << setw(10) << "Superf."
             << setw(7) << "Pista" << setw(8) << "Sector" << setw(8) << "Offset"
             << setw(7) << "Tam." << setw(8) << "Ocupado" << endl;
        cout << string(60, '-') << endl;
        for (const auto& rm : diccionarioDeDatosEnRAM) {
            cout << setw(5) << rm.idRegistro << setw(8) << rm.platoIdx << setw(10) << rm.superficieIdx
                 << setw(7) << rm.pistaIdx << setw(8) << rm.sectorGlobalEnPista << setw(8) << rm.offset
                 << setw(7) << rm.tamRegistro << setw(8) << (rm.ocupado ? "Si" : "No") << endl;
        }
        cout << "-----------------------------------------------\n";
    }

    string getTablaEsquema() const {
        return tablaEsquema;
    }
};

// Función para mostrar el menú
void mostrarMenu() {
    cout << "\n--- Sistema de Gestión de Almacenamiento ---\n";
    cout << "1. Crear nuevo disco\n";
    cout << "2. Cargar disco existente\n";
    cout << "3. Cargar datos desde CSV\n";
    cout << "4. Insertar nuevo registro\n";
    cout << "5. Recuperar registro por ID\n";
    cout << "6. Eliminar registro por ID\n";
    cout << "7. Mostrar mapa de bits de sectores\n";
    cout << "8. Mostrar estado del diccionario de datos\n";
    cout << "9. Salir\n";
    cout << "Ingrese su opción: ";
}

int main() {
    Disco* disco = nullptr;
    int opcion;

    // Crear la carpeta base para los discos si no existe
    MKDIR("Discos");

    do {
        mostrarMenu();
        cin >> opcion;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar el buffer de entrada

        switch (opcion) {
            case 1: { // Crear nuevo disco
                int nPlatos, nSuperficies, nPistas, nSectores, capSector;
                string nombreDisco;

                cout << "Ingrese nombre del nuevo disco: ";
                getline(cin, nombreDisco);
                cout << "Número de platos: ";
                cin >> nPlatos;
                cout << "Número de superficies por plato: ";
                cin >> nSuperficies;
                cout << "Número de pistas por superficie: ";
                cin >> nPistas;
                cout << "Número de sectores por pista: ";
                cin >> nSectores;
                cout << "Capacidad de cada sector (bytes): ";
                cin >> capSector;
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar

                if (disco != nullptr) {
                    delete disco; // Liberar memoria del disco anterior si existe
                }
                disco = new Disco(nPlatos, nSuperficies, nPistas, nSectores, capSector, nombreDisco);
                cout << "Disco '" << nombreDisco << "' creado exitosamente." << endl;
                break;
            }
            case 2: { // Cargar disco existente
                string rutaDisco;
                cout << "Ingrese la ruta del disco a cargar (ej. './MiDisco_disk'): ";
                getline(cin, rutaDisco);
                if (disco != nullptr) {
                    delete disco;
                }
                disco = Disco::cargarDisco(rutaDisco);
                if (disco == nullptr) {
                    cout << "Error al cargar el disco." << endl;
                }
                break;
            }
            case 3: { // Cargar datos desde CSV
                if (disco == nullptr) {
                    cout << "Primero debe crear o cargar un disco (opción 1 o 2).\n";
                    break;
                }
                string rutaCSV;
                cout << "Ingrese la ruta del archivo CSV a cargar: ";
                getline(cin, rutaCSV);
                disco->cargarCSV(rutaCSV);
                break;
            }
            case 4: { // Insertar nuevo registro
                if (disco == nullptr) {
                    cout << "Primero debe crear o cargar un disco (opción 1 o 2).\n";
                    break;
                }
                string esquema = disco->getTablaEsquema();
                if (esquema.empty()) {
                    cout << "No hay esquema de tabla. Cargue un CSV primero para definir el esquema.\n";
                    break;
                }
                cout << "Esquema actual: " << esquema << "\n";
                cout << "Ingrese los datos del nuevo registro, separados por '#': ";
                string nuevoRegistro;
                getline(cin, nuevoRegistro);
                disco->insertarRegistro(nuevoRegistro);
                break;
            }

            case 5: { // Recuperar registro por ID
                if (disco == nullptr) {
                    cout << "Primero debe crear o cargar un disco (opción 1 o 2).\n";
                    break;
                }
                long idToRetrieve;
                cout << "Ingrese el ID del registro a recuperar: ";
                cin >> idToRetrieve;
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar
                string registroRecuperado = disco->recuperarRegistro(idToRetrieve);
                if (!registroRecuperado.empty()) {
                    cout << "Registro recuperado: " << registroRecuperado << endl;
                } else {
                    cout << "Registro ID " << idToRetrieve << " no encontrado o eliminado." << endl;
                }
                break;
            }
            case 6: { // Eliminar registro por ID
                if (disco == nullptr) {
                    cout << "Primero debe crear o cargar un disco (opción 1 o 2).\n";
                    break;
                }
                long idToDelete;
                cout << "Ingrese el ID del registro a eliminar: ";
                cin >> idToDelete;
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar
                disco->eliminarRegistro(idToDelete);
                break;
            }

            case 7: { // Mostrar mapa de bits
                if (disco == nullptr) {
                    cout << "Primero debe crear o cargar un disco (opción 1 o 2).\n";
                    break;
                }
                disco->mostrarMapaDeBits();
                break;
            }
            case 8: { // Mostrar estado del diccionario de datos
                if (disco == nullptr) {
                    cout << "Primero debe crear o cargar un disco (opción 1 o 2).\n";
                    break;
                }
                disco->mostrarEstadoDiccionario();
                break;
            }

            case 9: // Salir
                cout << "Saliendo...\n";
                break;

            default:
                cout << "Opción inválida, intente de nuevo.\n";
        }

    } while (opcion != 9);

    if (disco != nullptr) {
        delete disco;
    }
    return 0;
}