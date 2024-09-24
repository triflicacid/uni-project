#pragma once

namespace language {
  class CompileData {
    private:
    bool m_registers[6]; // Mark true/false which registers r# are in use
    int m_latest; // Index of latest register used
    int m_sp_offset; // Offset from SP

    public:
    CompileData() : m_latest(-1), m_sp_offset(0) {
      for (bool &m_register: m_registers) {
        m_register = false;
      }
    }

    /** Check if the given register is free. */
    [[nodiscard]] bool is_free(int i) const { return !m_registers[i]; }

    /** Ask for free register. */
    int get_register() {
      for (int i = 0; i < sizeof(m_registers); i++) {
        if (!m_registers[i]) {
          return m_latest = i;
        }
      }

      return 0;
    }

    /** Get latest retrieved register. */
    [[nodiscard]] int latest_register() const { return m_latest; }

    /** Free the given register. */
    void free_register(int i) { m_registers[i] = false; }

    /** Get SP offset */
    [[nodiscard]] int stack_offset() const { return m_sp_offset; }

    void set_stack_offset(int x) { m_sp_offset = x; }

    void add_stack_offset(int x) { m_sp_offset += x; }
  };
}
