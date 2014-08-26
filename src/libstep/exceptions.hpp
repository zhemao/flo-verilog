namespace libstep {
    class nofile_exception : public std::exception {
        const char* what() const throw() { return "File not found"; }
    };
    class malformed_exception : public std::exception {
        const char* what() const throw() { return "Malformed step file"; }
    };
}
