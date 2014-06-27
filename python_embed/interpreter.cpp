#include <string>
#include <iostream>
#include <boost/python.hpp>
#include <boost/filesystem.hpp>
#include "api.h"

namespace py = boost::python;

int main(int argc, char **argv) {
    Py_Initialize();

    // All Python errors should result in a Python traceback

    try {
        // Get the current directory and add (append) it to Python's sys.path
        auto workingDir = boost::filesystem::absolute("./").normalize();
        auto sys_paths = py::import("sys").attr("path");
        sys_paths.attr("append")(py::str(workingDir.string()));

        // The main module's namespace is needed to evaluate code
        // and the main module object is needed to have
        // juicy, juicy introspection and monkey-patching.
        auto main_module = py::import("__main__");
        auto main_namespace = main_module.attr("__dict__");

        // Expose the wrappers to Python code.
        // This will be wrapped better in the final thing, with
        // a Python module to abstract C-level errors. 
        auto wrapper_module = py::import("wrapper_functions");
        main_module.attr("wrapper_functions") = wrapper_module;

        // Inject a player; will be instantiated in C++
        auto my_player = Player(Vec2D(90, 102), Direction::UP, "John");
        main_module.attr("preplayer") = py::object(boost::ref(my_player));

        // Pass the Vec2D class. Should be 
        main_module.attr("Vec2D") = py::object(Vec2D(0, 0)).attr("__class__");

        // Run the wrapper Python code
        py::exec(
            "print('--- Inside ---')\n"
            "preplayer.monologue()\n"
            "preplayer.move(Vec2D(10, 14))\n"
            "preplayer.monologue()\n"

            "def script(player):\n"
            "    x_direction = Vec2D(1, 0)\n"
            "    for _ in range(10000):\n"
            "        player.move(x_direction)\n"
            "    player.monologue()\n"

            "preplayer.give_script(script)\n"
            "preplayer.run_script()\n"
            "preplayer.monologue()\n"
            ,
            main_namespace
        );

        // Prove that this has affected the outside environment
        std::cout << "--- Outside ---" << std::endl;
        my_player.monologue();
    }
    catch (boost::python::error_already_set) {
        PyErr_Print();
    }
}