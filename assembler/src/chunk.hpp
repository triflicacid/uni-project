#pragma once

#include <utility>

#include "instructions/instruction.hpp"

namespace assembler {
  /** @brief A single unit of assembled output at a fixed byte offset: one instruction, a data blob, or a reserved space directive. */
  class Chunk {
  public:
    uint32_t offset; ///< Byte offset this chunk occupies in the output.

  private:
    Location m_source; ///< Source location this chunk was assembled from.
//    std::unique_ptr<instruction::Instruction> m_instruction = nullptr;
//    std::unique_ptr<std::vector<uint8_t>> m_bytes = nullptr;

  public:
    /**
     * @brief Construct a chunk.
     * @param source Source location this chunk was assembled from.
     * @param offset Byte offset this chunk occupies in the output.
     */
    Chunk(Location source, uint32_t offset) : m_source(std::move(source)), offset(offset) {}

    virtual ~Chunk() = default;

    /**
     * @brief Print a human-readable description of this chunk for debugging.
     * @param os Stream to print to.
     */
    virtual void debug_print(std::ostream &os);

    /** @brief Get the chunk's size in bytes. @return Size in bytes. */
    [[nodiscard]] virtual uint16_t size() const = 0;

    /** @brief Get the source location this chunk was assembled from. @return The source location. */
    [[nodiscard]] const Location &location() const { return m_source; }

    /**
     * @brief Write this chunk's compiled bytes to a stream.
     * @param os Stream to write to.
     */
    virtual void write(std::ostream &os) = 0;

    /**
     * @brief Write this chunk back out as reconstructed assembly text.
     * @param os Stream to write to.
     */
    virtual void reconstruct(std::ostream &os) = 0;

    /**
     * @brief Replace every reference to a label within this chunk with its resolved address.
     * @param label Label name to replace.
     * @param address Address to replace it with.
     * @param debug If true, print debug information about the replacement.
     */
    virtual void replace_label(const std::string& label, uint32_t address, bool debug = false) {}

    /** @brief Get the first unresolved label argument in this chunk, if any. @return The label argument, or nullptr if none. */
    virtual const instruction::ArgumentLabel* get_first_label() const
    { return nullptr; }
  };

  /** @brief A chunk holding a single encoded instruction. */
  class InstructionChunk : public Chunk {
    std::unique_ptr<instruction::Instruction> m_instruction; ///< The encoded instruction this chunk holds.

  public:
    /**
     * @brief Construct an instruction chunk.
     * @param source Source location this chunk was assembled from.
     * @param offset Byte offset this chunk occupies in the output.
     * @param i Instruction this chunk encodes.
     */
    InstructionChunk(Location source, uint32_t offset, std::unique_ptr<instruction::Instruction> i)
      : Chunk(std::move(source), offset), m_instruction(std::move(i)) {}

    /** @brief Get the chunk's size in bytes (always one instruction word). @return `sizeof(uint64_t)`. */
    uint16_t size() const override { return sizeof(uint64_t); }

    /** @brief Print a human-readable description of the instruction for debugging. @param os Stream to print to. */
    void debug_print(std::ostream &os) override;

    /** @brief Write the encoded instruction word to a stream. @param os Stream to write to. */
    void write(std::ostream &os) override;

    /** @brief Write the instruction back out as reconstructed assembly text. @param os Stream to write to. */
    void reconstruct(std::ostream &os) override;

    /**
     * @brief Replace every reference to a label within this instruction's arguments with its resolved address.
     * @param label Label name to replace.
     * @param address Address to replace it with.
     * @param debug If true, print debug information about the replacement.
     */
    void replace_label(const std::string &label, uint32_t address, bool debug = false) override;

    /** @brief Get this instruction's first unresolved label argument, if any. @return The label argument, or nullptr if none. */
    const instruction::ArgumentLabel* get_first_label() const override;
  };

  /** @brief A chunk holding a raw block of data bytes (e.g. from a `.data`-style directive). */
  class DataChunk : public Chunk {
    std::vector<uint8_t> m_bytes; ///< Raw data bytes this chunk holds.

  public:
    /**
     * @brief Construct a data chunk.
     * @param source Source location this chunk was assembled from.
     * @param offset Byte offset this chunk occupies in the output.
     * @param bytes Raw bytes to store.
     */
    DataChunk(Location source, uint32_t offset, std::vector<uint8_t> bytes)
      : Chunk(std::move(source), offset), m_bytes(std::move(bytes)) {}

      /** @brief Get the chunk's size in bytes. @return Number of stored bytes. */
      uint16_t size() const override;

      /** @brief Print a human-readable description of the data for debugging. @param os Stream to print to. */
      void debug_print(std::ostream &os) override;

      /** @brief Write the raw bytes to a stream. @param os Stream to write to. */
      void write(std::ostream &os) override;

      /** @brief Write the data back out as reconstructed assembly text. @param os Stream to write to. */
      void reconstruct(std::ostream &os) override;
  };

  /** @brief A chunk that reserves a run of zero-initialised bytes (e.g. from a `.space`-style directive), without storing content. */
  class SpaceDirectiveChunk : public Chunk {
    uint32_t m_value; ///< Number of zero bytes to reserve.

  public:
    /**
     * @brief Construct a space-reservation chunk.
     * @param source Source location this chunk was assembled from.
     * @param offset Byte offset this chunk occupies in the output.
     * @param value Number of bytes to reserve.
     */
    SpaceDirectiveChunk(Location source, uint32_t offset, uint32_t value)
      : Chunk(std::move(source), offset), m_value(value) {}

    /** @brief Get the chunk's size in bytes. @return Number of reserved bytes. */
    uint16_t size() const override;

    /** @brief Print a human-readable description of the reservation for debugging. @param os Stream to print to. */
    void debug_print(std::ostream &os) override;

    /** @brief Write zero bytes for the reserved space to a stream. @param os Stream to write to. */
    void write(std::ostream &os) override;

    /** @brief Write the reservation back out as reconstructed assembly text. @param os Stream to write to. */
    void reconstruct(std::ostream &os) override;
  };
}
