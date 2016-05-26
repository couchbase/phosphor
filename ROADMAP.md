# Roadmap

- Improve iterator to return an object with thread context
- Refactor TraceLog::logEvent to not duplicate code
- Add chunked JSON export (Which takes an iterator)
- Add binary dump export

## Performance Idea List

For performance improvements that have been suggested or considered but are
currently premature:

- Check enabled flag with relaxed atomic
- Try avoiding the conditional branch caused by the ternary expression in
TraceLog::logEvent
- Use \__builtin_expect where useful
- Sentinel benchmarking suggests that double the number of logical cores
is the sweet-spot for the size of a shared array of ChunkTenants.