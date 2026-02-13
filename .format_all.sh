#!/bin/bash
find ./src -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.ipp" -o -name "*.tpp" \) | while read -r file; do
    nvim --headless \
         -c "edit $file" \
         -c "lua vim.wait(2000, function() return #vim.lsp.get_active_clients({bufnr=0}) > 0 end)" \
         -c "lua vim.lsp.buf.format({async=false})" \
         -c "write" \
         -c "quit" \
         2>/dev/null
    echo "Formatted: $file"
done
