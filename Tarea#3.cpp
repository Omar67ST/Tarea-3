#include <iostream>
#include <string>
#include <list>
#include <map>
#include <functional>
#include <cstdlib>

using namespace std;

// Tipo de comando: función que recibe una lista de argumentos
using Command = function<void(const list<string>&)>;

class Entity {
    // Atributos privados
    float x, y;
    float health;
    float energy;
    string name;
    int level;
    int resources;

public: //Constructor General
    Entity(float x, float y, float health, float energy, string name, int level, int resources) 
        : x(x), y(y), health(health), energy(energy), name(name), level(level), resources(resources) {}

    // Getters
    string getName() const { return name; }
    float getHealth() const { return health; }
    float getEnergy() const { return energy; }
    float getX() const { return x; }
    float getY() const { return y; }
    int getLevel() const { return level; }
    int getResources() const { return resources; }

    // Modificadores
    void move(float newX, float newY) {
        x = newX;
        y = newY;
    }

    void heal(float amount) {
        health += amount;
    }

    void damage(float amount) {
        health -= amount;
        if (health < 0) health = 0; // Evitar salud negativa
    }

    void useEnergy(float amount) {
        energy -= amount;
        if (energy < 0) energy = 0; // Evitar energía negativa
    }

    void restoreEnergy(float amount) {
        energy += amount;
    }

    void gainResources(int amount) {
        resources += amount;
    }

    void levelUp() {
        level++;
    }
};

// Comando como función libre 
void cmd_reset(const list<string>& args, Entity& entity) {
    entity.move(0, 0);
    entity.heal(100);
    entity.restoreEnergy(50);
    cout << entity.getName() << " ha sido reseteado a posición (0,0), salud +100, energía +50." << endl;
}

// Modelo de Comando 
class CommandCenter {
    map<string, Command> commands;
    list<string> history; // Historial de ejecución
    Entity& entity;       // Instancia compartida de Entity

public:
    CommandCenter(Entity& e) : entity(e) {}

    // Registrar un comando con un nombre clave
    void registerCommand(const string& name, Command cmd) {
        commands[name] = cmd;
    }

    // Ejecutar un comando por nombre, pasándole los argumentos
    void execute(const string& name, const list<string>& args) {
        // Usar iterador explícito (requisito del PDF)
        map<string, Command>::iterator it = commands.find(name);
        if (it != commands.end()) {
            // Guardar estado antes de ejecutar
            string estadoAntes = "HP:" + to_string((int)entity.getHealth()) +
                                 " EN:" + to_string((int)entity.getEnergy()) +
                                 " LVL:" + to_string(entity.getLevel()) +
                                 " REC:" + to_string(entity.getResources());
            it->second(args);

            // Guardar estado después de ejecutar
            string estadoDespues = "HP:" + to_string((int)entity.getHealth()) +
                                   " EN:" + to_string((int)entity.getEnergy()) +
                                   " LVL:" + to_string(entity.getLevel()) +
                                   " REC:" + to_string(entity.getResources());
            history.push_back(name + " [" + estadoAntes + " -> " + estadoDespues + "]");
        } else {
            cout << "Comando no encontrado: " << name << endl;
        }
    }

    // Elimina un comando dinámicamente
    void unregisterCommand(const string& name) {
        // Usar iterador explícito
        map<string, Command>::iterator it = commands.find(name);
        if (it != commands.end()) {
            commands.erase(it);
            cout << "Comando '" << name << "' eliminado." << endl;
        } else {
            cout << "Comando '" << name << "' no existe, no se puede eliminar." << endl;
        }
    }

    // Registrar un macro comando
    void registerMacro(
        const string& name,
        const list<pair<string, list<string>>>& steps
    ) {
        // Guardamos el macro como un Command en el mismo mapa
        commands[name] = [this, name, steps](const list<string>& args) {
            cout << "Ejecutando macro: " << name << endl;
            list<pair<string, list<string>>>::const_iterator it = steps.begin();
            while (it != steps.end()) {
                const string& cmdName = it->first;
                const list<string>& cmdArgs = it->second;
                map<string, Command>::iterator cmdIt = commands.find(cmdName);
                if (cmdIt != commands.end()) {
                    cmdIt->second(cmdArgs);
                } else {
                    cout << "Error: comando '" << cmdName << "' no existe en macro '" << name << "'." << endl;
                    return; // Detener ejecución si falla un paso
                }
                ++it;
            }
        };
    }

    // Ejecutar un macro comando por nombre
    void executeMacro(const string& name) {
        map<string, Command>::iterator it = commands.find(name);
        if (it != commands.end()) {
            it->second({});
            history.push_back("MACRO:" + name);
        } else {
            cout << "Macro no encontrado: " << name << endl;
        }
    }

    // Muestra el historial de comandos ejecutados
    void printHistory() const {
        cout << "---Historial de ejecución---" << endl;
        list<string>::const_iterator it = history.begin();
        while (it != history.end()) {
            cout << " - " << *it << endl;
            ++it;
        }
    }

    // Referencia a la entidad
    Entity& getEntity() {
        return entity;
    }

    // Comandos compuestos (Macro Commands)
    class MacroCommand {
        CommandCenter& center;
        list<pair<string, list<string>>> steps;

    public:
        MacroCommand(CommandCenter& c) : center(c) {}

        void addStep(const string& cmdName, const list<string>& args) {
            steps.push_back({cmdName, args});
        }

        void operator()(const list<string>& args) {
            cout << "Ejecutando MacroCommand..." << endl;
            list<pair<string, list<string>>>::const_iterator it = steps.begin();
            while (it != steps.end()) {
                cout << " -> " << it->first << endl;
                center.execute(it->first, it->second);
                ++it;
            }
        }
    };
};  

class HealFunctor {
    Entity& entity;
    int useCount;
    int maxUses;

public:
    HealFunctor(Entity& e, int maxUses) : entity(e), useCount(0), maxUses(maxUses) {}

    void operator()(const list<string>& args) {
        if (useCount >= maxUses) {
            cout << "Heal ha alcanzado su límite de " << maxUses << " usos." << endl;
            return;
        }
        if (args.empty()) {
            cout << "healLimited requiere un argumento numérico." << endl;
            return;
        }
        try {
            float amount = stof(args.front());
            if (amount <= 0) { cout << "El valor debe ser positivo." << endl; return; }
            entity.heal(amount);
            useCount++;
            cout << entity.getName() << " curado en " << amount
                 << " HP. (Uso " << useCount << "/" << maxUses << ")" << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido: se esperaba un número." << endl;
        }
    }
};

// Functor para ganar recursos con multiplicador 
class GainResourcesFunctor {
    Entity& entity;
    int multiplier;
    int totalGained;

public:
    GainResourcesFunctor(Entity& e, int mult) : entity(e), multiplier(mult), totalGained(0) {}

    void operator()(const list<string>& args) {
        if (args.empty()) { cout << "gainResourcesX requiere un argumento." << endl; return; }
        try {
            int amount = stoi(args.front()) * multiplier;
            entity.gainResources(amount);
            totalGained += amount;
            cout << entity.getName() << " ganó " << amount
                 << " recursos (x" << multiplier << "). Total acumulado: " << totalGained << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido: se esperaba un entero." << endl;
        }
    }
};

// Functor de daño con contador de veces aplicado
class DamageFunctor {
    Entity& entity;
    int timesUsed;

public:
    DamageFunctor(Entity& e) : entity(e), timesUsed(0) {}

    void operator()(const list<string>& args) {
        if (args.empty()) { cout << "damageFunctor requiere un argumento." << endl; return; }
        try {
            float amount = stof(args.front());
            entity.damage(amount);
            timesUsed++;
            cout << entity.getName() << " recibió " << amount
                 << " de daño (aplicado " << timesUsed << " veces en total)." << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido: se esperaba un número." << endl;
        }
    }
};

// Registro de Comandos
void registerDefaultCommands(CommandCenter& center, Entity& entity) {

    // --- Comandos como lambdas 
    center.registerCommand("heal", [&entity](const list<string>& args) {
        if (args.empty()) { cout << "heal requiere un argumento." << endl; return; }
        try {
            float amount = stof(args.front());
            if (amount <= 0) { cout << "El valor debe ser positivo." << endl; return; }
            entity.heal(amount);
            cout << entity.getName() << " curado en " << amount << " HP." << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido para heal." << endl;
        }
    });

    center.registerCommand("damage", [&entity](const list<string>& args) {
        if (args.empty()) { cout << "damage requiere un argumento." << endl; return; }
        try {
            float amount = stof(args.front());
            if (amount <= 0) { cout << "El valor debe ser positivo." << endl; return; }
            entity.damage(amount);
            cout << entity.getName() << " recibió " << amount << " de daño." << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido para damage." << endl;
        }
    });

    center.registerCommand("move", [&entity](const list<string>& args) {
        if (args.size() < 2) { cout << "move requiere 2 argumentos." << endl; return; }
        try {
            float newX = stof(args.front());
            float newY = stof(*next(args.begin()));
            entity.move(newX, newY);
            cout << entity.getName() << " se movió a (" << newX << ", " << newY << ")." << endl;
        } catch (const invalid_argument&) {
            cout << "Argumentos inválidos para move." << endl;
        }
    });

    center.registerCommand("useEnergy", [&entity](const list<string>& args) {
        if (args.empty()) { cout << "useEnergy requiere un argumento." << endl; return; }
        try {
            float amount = stof(args.front());
            if (amount <= 0) { cout << "El valor debe ser positivo." << endl; return; }
            entity.useEnergy(amount);
            cout << entity.getName() << " usó " << amount << " de energía." << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido para useEnergy." << endl;
        }
    });

    center.registerCommand("gainResources", [&entity](const list<string>& args) {
        if (args.empty()) { cout << "gainResources requiere un argumento." << endl; return; }
        try {
            int amount = stoi(args.front());
            if (amount <= 0) { cout << "El valor debe ser positivo." << endl; return; }
            entity.gainResources(amount);
            cout << entity.getName() << " ganó " << amount << " recursos." << endl;
        } catch (const invalid_argument&) {
            cout << "Argumento inválido para gainResources." << endl;
        }
    });

    center.registerCommand("levelUp", [&entity](const list<string>& args) {
        entity.levelUp();
        cout << entity.getName() << " subió al nivel " << entity.getLevel() << "." << endl;
    });

    center.registerCommand("status", [&entity](const list<string>& args) {
        cout << "---Estado de " << entity.getName() << "---" << endl;
        cout << "Posición: (" << entity.getX() << ", " << entity.getY() << ")" << endl;
        cout << "Salud: " << entity.getHealth() << endl;
        cout << "Energía: " << entity.getEnergy() << endl;
        cout << "Nivel: " << entity.getLevel() << endl;
        cout << "Recursos: " << entity.getResources() << endl;
    });

    // Comando como función libre
    center.registerCommand("reset", [&entity](const list<string>& args) {
        cmd_reset(args, entity);
    });

    // Functor con límite de 3 usos 
    center.registerCommand("healLimited", HealFunctor(entity, 3));

    // Functor con multiplicador de recursos 
    center.registerCommand("gainResourcesX2", GainResourcesFunctor(entity, 2));

    // Functor de daño con contador 
    center.registerCommand("damageFunctor", DamageFunctor(entity));
}

// main
int main() {
    // Creación de la entidad
    Entity player(0, 0, 100, 50, "Heroe", 1, 10);

    // Creación del CommandCenter
    CommandCenter center(player);

    // Registro de comandos básicos
    registerDefaultCommands(center, player);

    // Ejecución de comandos válidos
    center.execute("status", {});
    center.execute("move", {"10", "20"});
    center.execute("heal", {"30"});
    center.execute("damage", {"15"});
    center.execute("useEnergy", {"20"});
    center.execute("gainResources", {"40"});
    center.execute("levelUp", {});
    center.execute("status", {});

    // Ejecución de comando inválido
    center.execute("comandoInexistente", {});

    // Ejecución de comando con argumento inválido (validación)
    center.execute("heal", {"abc"});
    center.execute("move", {"5"});  // falta segundo argumento

    cout << "\n=== Demostracion de functores ===" << endl;

    // Demostración: healLimited (máximo 3 usos)
    center.execute("healLimited", {"10"});
    center.execute("healLimited", {"10"});
    center.execute("healLimited", {"10"});
    center.execute("healLimited", {"10"}); // debe mostrar límite alcanzado

    // Demostración: gainResourcesX2 (functor con multiplicador)
    center.execute("gainResourcesX2", {"15"});
    center.execute("gainResourcesX2", {"20"});
    center.execute("gainResourcesX2", {"5"});

    // Demostración: damageFunctor (functor con contador)
    center.execute("damageFunctor", {"8"});
    center.execute("damageFunctor", {"12"});
    center.execute("damageFunctor", {"5"});

    cout << "\n=== Demostracion de funcion libre (reset) ===" << endl;
    center.execute("reset", {});
    center.execute("status", {});

    // Registro de macros (al menos 3)
    cout << "\n=== Registro y ejecucion de macros ===" << endl;

    list<pair<string, list<string>>> healAndStatus = {
        {"heal", {"10"}},
        {"status", {}}
    };
    center.registerMacro("heal_and_status", healAndStatus);

    list<pair<string, list<string>>> moveAndDamage = {
        {"move", {"5", "5"}},
        {"damage", {"20"}}
    };
    center.registerMacro("move_and_damage", moveAndDamage);

    list<pair<string, list<string>>> levelUpResources = {
        {"levelUp", {}},
        {"gainResources", {"50"}}
    };
    center.registerMacro("levelup_and_resources", levelUpResources);

    // Ejecución de macros
    center.executeMacro("heal_and_status");
    center.executeMacro("move_and_damage");
    center.executeMacro("levelup_and_resources");

    // Ejecución de macro inexistente
    center.executeMacro("macroInexistente");

    cout << "\n=== Demostracion de MacroCommand (clase interna) ===" << endl;

    // Uso de la clase interna MacroCommand
    // Ejemplo 1
    CommandCenter::MacroCommand macro1(center);
    macro1.addStep("heal", {"25"});
    macro1.addStep("useEnergy", {"10"});
    macro1.addStep("status", {});
    center.registerCommand("macro_heal_energy_status", macro1);
    center.execute("macro_heal_energy_status", {});

    // Ejemplo 2
    CommandCenter::MacroCommand macro2(center);
    macro2.addStep("levelUp", {});
    macro2.addStep("gainResources", {"100"});
    macro2.addStep("status", {});
    center.registerCommand("macro_levelup_rich", macro2);
    center.execute("macro_levelup_rich", {});

    // Ejemplo 3
    CommandCenter::MacroCommand macro3(center);
    macro3.addStep("move", {"30", "40"});
    macro3.addStep("damage", {"5"});
    macro3.addStep("status", {});
    center.registerCommand("macro_explore", macro3);
    center.execute("macro_explore", {});

    cout << "\n=== Eliminacion dinamica de comandos ===" << endl;

    // Eliminación dinámica de comandos
    center.unregisterCommand("damage");
    center.execute("damage", {"10"}); // debe indicar que no existe

    center.unregisterCommand("useEnergy");
    center.execute("useEnergy", {"5"}); // debe indicar que no existe

    center.unregisterCommand("comandoQueNoExiste"); // debe indicar que no existe

    // Visualización del estado final
    cout << "\n=== Estado final ===" << endl;
    center.execute("status", {});

    // Mostrar historial de ejecución
    cout << endl;
    center.printHistory();

    return 0;
}