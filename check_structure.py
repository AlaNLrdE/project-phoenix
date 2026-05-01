#!/usr/bin/env python3
"""
PROJECT PHOENIX - QUICK REFERENCE & STRUCTURE VERIFICATION
Verifica que toda la estructura de Phase 1 está presente
"""

import os
import sys
from pathlib import Path

# Define la estructura esperada
EXPECTED_STRUCTURE = {
    'include/': ['math/', 'physics/', 'vessels/', 'world/'],
    'include/math/': ['Constants.hpp'],
    'include/physics/': ['Orbit.hpp', 'CelestialBody.hpp'],
    'include/vessels/': ['Vessel.hpp'],
    'include/world/': ['WorldManager.hpp'],
    'src/': ['main.cpp', 'physics/', 'vessels/', 'world/'],
    'src/physics/': ['Orbit.cpp', 'CelestialBody.cpp'],
    'src/vessels/': ['Vessel.cpp'],
    'src/world/': ['WorldManager.cpp'],
    '.vscode/': ['launch.json', 'tasks.json', 'settings.json'],
    '.': ['CMakeLists.txt', 'README.md', 'ROADMAP.md', 'build.sh', '.gitignore']
}

def check_structure(root_path):
    """Verifica la estructura del proyecto"""
    
    print("\n╔════════════════════════════════════════╗")
    print("║  PROJECT PHOENIX - STRUCTURE CHECK    ║")
    print("╚════════════════════════════════════════╝\n")
    
    root = Path(root_path)
    all_ok = True
    
    for directory, items in EXPECTED_STRUCTURE.items():
        dir_path = root / directory if directory != '.' else root
        
        if not dir_path.exists():
            print(f"✗ {directory} - MISSING")
            all_ok = False
            continue
        
        print(f"✓ {directory}")
        
        for item in items:
            item_path = dir_path / item
            
            if not item_path.exists():
                print(f"  ✗ {item} - MISSING")
                all_ok = False
            else:
                if item.endswith('/'):
                    print(f"  ✓ {item} (directory)")
                else:
                    size = item_path.stat().st_size
                    print(f"  ✓ {item} ({size} bytes)")
    
    print("\n" + "="*40)
    
    if all_ok:
        print("✓ Project structure is COMPLETE")
        return 0
    else:
        print("✗ Some files are MISSING")
        return 1

def print_quick_start():
    """Imprime instrucciones de inicio rápido"""
    
    print("\n╔════════════════════════════════════════╗")
    print("║        QUICK START GUIDE              ║")
    print("╚════════════════════════════════════════╝\n")
    
    print("1. CHECK DEPENDENCIES:")
    print("   $ apt-cache search libglm-dev")
    print("   $ apt-get install libglm-dev cmake g++\n")
    
    print("2. BUILD PROJECT:")
    print("   $ chmod +x build.sh")
    print("   $ ./build.sh release          # Release build")
    print("   $ ./build.sh debug            # Debug build")
    print("   $ ./build.sh run              # Build & run\n")
    
    print("3. BUILD WITH CMAKE:")
    print("   $ mkdir build && cd build")
    print("   $ cmake -DCMAKE_BUILD_TYPE=Release ..")
    print("   $ make -j$(nproc)")
    print("   $ ./phoenix\n")
    
    print("4. VS CODE:")
    print("   • Open folder in VS Code")
    print("   • Use Ctrl+Shift+B to build")
    print("   • Use F5 to debug\n")

def print_module_summary():
    """Imprime resumen de módulos"""
    
    print("\n╔════════════════════════════════════════╗")
    print("║      MODULE SUMMARY & RESPONSIBILITIES║")
    print("╚════════════════════════════════════════╝\n")
    
    modules = [
        ("math/Constants.hpp", 
         "Constantes astrodinámica, tipos GLM, conversiones"),
        ("physics/Orbit.hpp/cpp",
         "Elementos orbitales, ecuación de Kepler, propagación"),
        ("physics/CelestialBody.hpp/cpp",
         "Cuerpos celestes, propiedades físicas, órbitas"),
        ("vessels/Vessel.hpp/cpp",
         "Naves espaciales, estado orbital, maniobras"),
        ("world/WorldManager.hpp/cpp",
         "Gestor central, simulación, time warp, registros"),
    ]
    
    for module, description in modules:
        print(f"• {module}")
        print(f"  └─ {description}\n")

def print_api_quick_reference():
    """Imprime referencia rápida de API"""
    
    print("\n╔════════════════════════════════════════╗")
    print("║     API QUICK REFERENCE               ║")
    print("╚════════════════════════════════════════╝\n")
    
    print("CREAR ÓRBITA:")
    print("  Orbit(a, e, i, Ω, ω, ν, μ, epoch)")
    print("  Orbit(position, velocity, μ, epoch)\n")
    
    print("PROPAGAR:")
    print("  dvec3 pos = orbit.getPositionAtTime(t)")
    print("  dvec3 vel = orbit.getVelocityAtTime(t)")
    print("  orbit.getStateAtTime(t, pos, vel)\n")
    
    print("CUERPO CELESTE:")
    print("  CelestialBody* earth = new CelestialBody(...)")
    print("  earth->setOrbit(orbit, parentName)")
    print("  earth->getPositionAtTime(t)\n")
    
    print("NAVE:")
    print("  Vessel* ship = new Vessel(name, mass, orbit, refName, refBody)")
    print("  ship->getPosition(t), ship->getVelocity(t)")
    print("  ship->getAltitude(), ship->applyDeltaV(dv, dir)\n")
    
    print("MUNDO:")
    print("  WorldManager world;")
    print("  world.registerCelestialBody(earth)")
    print("  world.registerVessel(ship)")
    print("  world.updateSimulation(dt)")
    print("  world.setTimeWarp(100)\n")

def main():
    root_path = os.path.dirname(os.path.abspath(__file__))
    
    # Verificar estructura
    result = check_structure(root_path)
    
    if result == 0:
        print_quick_start()
        print_module_summary()
        print_api_quick_reference()
        
        print("\n╔════════════════════════════════════════╗")
        print("║  PROJECT READY FOR COMPILATION       ║")
        print("║  See README.md and ROADMAP.md for    ║")
        print("║  detailed documentation              ║")
        print("╚════════════════════════════════════════╝\n")
    
    return result

if __name__ == '__main__':
    sys.exit(main())
