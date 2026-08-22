#!/bin/bash

# Script de compilação rápida para o Felsic Notes (C++)

# Cria a pasta de compilação se ela não existir
mkdir -p build

# Entra na pasta
cd build

# Configura o projeto com CMake (somente necessário na primeira vez ou quando mudar o CMakeLists)
echo "=> Configurando o CMake..."
cmake ..

# Compila usando todos os núcleos do processador para ser mais rápido
echo "=> Compilando..."
make -j$(nproc)

# Se a compilação der sucesso, avisa o usuário e tenta rodar
if [ $? -eq 0 ]; then
    echo -e "\n✅ Compilação concluída com sucesso!"
    echo "Você pode executar o app rodando: ./build/FelsicNotes"
    echo -e "----------------------------------------\n"
    
    # Inicia o app automaticamente (opcional)
    # ./FelsicNotes
else
    echo -e "\n❌ Erro durante a compilação."
fi
