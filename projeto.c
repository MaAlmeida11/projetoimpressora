#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/*
 * Controle da impressora Elgin (i8 / i9) em C, usando a DLL E1_Impressora01.dll.
 *
 * A DLL precisa estar na RAIZ do projeto (na mesma pasta de onde o programa e executado),
 * junto com os arquivos XMLSAT.xml e CANC_SAT.xml.
 *
 * IMPORTANTE: a DLL precisa ter a mesma arquitetura do compilador:
 *   - compilador de 32 bits (ex.: MinGW do Code::Blocks)  -> use a DLL da pasta x86
 *   - compilador de 64 bits (ex.: MSYS2 mingw64)          -> use a DLL da pasta x64
 */

/* ===================== Ligacao com a DLL (equivalente a interface ImpressoraDLL do Java) ===================== */

/* Cada typedef descreve a "forma" de um comando da DLL: o que devolve e quais parametros recebe */
typedef int (*FnAbreConexaoImpressora)(int tipo, const char *modelo, const char *conexao, int param);
typedef int (*FnFechaConexaoImpressora)(void);
typedef int (*FnImpressaoTexto)(const char *dados, int posicao, int estilo, int tamanho);
typedef int (*FnCorte)(int avanco);
typedef int (*FnImpressaoQRCode)(const char *dados, int tamanho, int nivelCorrecao);
typedef int (*FnImpressaoCodigoBarras)(int tipo, const char *dados, int altura, int largura, int HRI);
typedef int (*FnAvancaPapel)(int linhas);
typedef int (*FnStatusImpressora)(int param);
typedef int (*FnAbreGavetaElgin)(int pino, int ti, int tf);
typedef int (*FnAbreGaveta)(int pino, int ti, int tf);
typedef int (*FnSinalSonoro)(int qtd, int tempoInicio, int tempoFim);
typedef int (*FnModoPagina)(void);
typedef int (*FnLimpaBufferModoPagina)(void);
typedef int (*FnImprimeModoPagina)(void);
typedef int (*FnModoPadrao)(void);
typedef int (*FnPosicaoImpressaoHorizontal)(int posicao);
typedef int (*FnPosicaoImpressaoVertical)(int posicao);
typedef int (*FnImprimeXMLSAT)(const char *dados, int param);
typedef int (*FnImprimeXMLCancelamentoSAT)(const char *dados, const char *assQRCode, int param);

/* Um ponteiro para cada comando. Depois de carregarDLL(), e so chamar: AbreConexaoImpressora(...) */
static FnAbreConexaoImpressora AbreConexaoImpressora;
static FnFechaConexaoImpressora FechaConexaoImpressora;
static FnImpressaoTexto ImpressaoTexto;
static FnCorte Corte;
static FnImpressaoQRCode ImpressaoQRCode;
static FnImpressaoCodigoBarras ImpressaoCodigoBarras;
static FnAvancaPapel AvancaPapel;
static FnStatusImpressora StatusImpressora;
static FnAbreGavetaElgin AbreGavetaElgin;
static FnAbreGaveta AbreGaveta;
static FnSinalSonoro SinalSonoro;
static FnModoPagina ModoPagina;
static FnLimpaBufferModoPagina LimpaBufferModoPagina;
static FnImprimeModoPagina ImprimeModoPagina;
static FnModoPadrao ModoPadrao;
static FnPosicaoImpressaoHorizontal PosicaoImpressaoHorizontal;
static FnPosicaoImpressaoVertical PosicaoImpressaoVertical;
static FnImprimeXMLSAT ImprimeXMLSAT;
static FnImprimeXMLCancelamentoSAT ImprimeXMLCancelamentoSAT;

static HMODULE dll = NULL;

int tentativa;

/* Procura um comando dentro da DLL. Devolve 1 se achou, 0 se nao achou. */
static int carregaFuncao(void *destino, const char *nome)
{
    FARPROC f = GetProcAddress(dll, nome);
    if (f == NULL)
    {
        printf("Comando '%s' nao encontrado na DLL.\n", nome);
        return 0;
    }
    *(FARPROC *)destino = f;
    return 1;
}

/* Carrega a DLL da raiz do projeto e todos os comandos. Devolve 1 se deu certo. */
static int carregarDLL(void)
{
    dll = LoadLibraryA("E1_Impressora01.dll");
    if (dll == NULL)
    {
        printf("Nao foi possivel carregar E1_Impressora01.dll (erro %lu).\n", GetLastError());
        printf("Confira: a DLL esta na raiz do projeto? Ela tem a mesma arquitetura (32/64 bits) do compilador?\n");
        return 0;
    }

    int ok = 1;
    ok &= carregaFuncao(&AbreConexaoImpressora, "AbreConexaoImpressora");
    ok &= carregaFuncao(&FechaConexaoImpressora, "FechaConexaoImpressora");
    ok &= carregaFuncao(&ImpressaoTexto, "ImpressaoTexto");
    ok &= carregaFuncao(&Corte, "Corte");
    ok &= carregaFuncao(&ImpressaoQRCode, "ImpressaoQRCode");
    ok &= carregaFuncao(&ImpressaoCodigoBarras, "ImpressaoCodigoBarras");
    ok &= carregaFuncao(&AvancaPapel, "AvancaPapel");
    ok &= carregaFuncao(&StatusImpressora, "StatusImpressora");
    ok &= carregaFuncao(&AbreGavetaElgin, "AbreGavetaElgin");
    ok &= carregaFuncao(&AbreGaveta, "AbreGaveta");
    ok &= carregaFuncao(&SinalSonoro, "SinalSonoro");
    ok &= carregaFuncao(&ModoPagina, "ModoPagina");
    ok &= carregaFuncao(&LimpaBufferModoPagina, "LimpaBufferModoPagina");
    ok &= carregaFuncao(&ImprimeModoPagina, "ImprimeModoPagina");
    ok &= carregaFuncao(&ModoPadrao, "ModoPadrao");
    ok &= carregaFuncao(&PosicaoImpressaoHorizontal, "PosicaoImpressaoHorizontal");
    ok &= carregaFuncao(&PosicaoImpressaoVertical, "PosicaoImpressaoVertical");
    ok &= carregaFuncao(&ImprimeXMLSAT, "ImprimeXMLSAT");
    ok &= carregaFuncao(&ImprimeXMLCancelamentoSAT, "ImprimeXMLCancelamentoSAT");
    return ok;
}

/* ===================== Variaveis do programa ===================== */

static int conexaoAberta = 0;  /* 0 = fechada, 1 = aberta */
static int tipo = 1;           /* 1 = USB, 2 = Serial, 3 = Rede... */
static char modelo[50] = "i9"; /* "i8" ou "i9" */
static char conexao[50] = "USB";
static int parametro = 0;

/* Chave do QR Code usada no cancelamento SAT */
static const char *ASS_QRCODE =
    "Q5DLkpdRijIRGY6YSSNsTWK1TztHL1vD0V1Jc4spo/CEUqICEb9SFy82ym8EhBRZjbh3btsZhF+sjHqEMR159i4agru9x6KsepK/"
    "q0E2e5xlU5cv3m1woYfgHyOkWDNcSdMsS6bBh2Bpq6s89yJ9Q6qh/J8YHi306ce9Tqb/drKvN2XdE5noRSS32TAWuaQEVd7u+"
    "TrvXlOQsE3fHR1D5f1saUwQLPSdIv01NF6Ny7jZwjCwv1uNDgGZONJdlTJ6p0ccqnZvuE70aHOI09elpjEO6Cd+orI7XHHrFCwhFhAcbalc+"
    "ZfO5b/+vkyAHS6CYVFCDtYR9Hi5qgdk31v23w==";

/* ===================== Funcoes auxiliares ===================== */

/* Mostra a mensagem e le uma linha de texto digitada (sem o ENTER no final) */
static void capturarEntrada(const char *mensagem, char *destino, int tamanho)
{
    printf("%s", mensagem);
    if (fgets(destino, tamanho, stdin) == NULL)
    {
        destino[0] = '\0';
        return;
    }
    destino[strcspn(destino, "\r\n")] = '\0';
}

/* Mostra a mensagem e le um numero inteiro. Se digitar letras, pergunta de novo. */
static int lerInteiro(const char *mensagem)
{
    char linha[50];
    char *fim;
    while (1)
    {
        capturarEntrada(mensagem, linha, sizeof(linha));
        long valor = strtol(linha, &fim, 10);
        if (linha[0] != '\0' && *fim == '\0')
        {
            return (int)valor;
        }
        printf("Digite apenas numeros!\n");
    }
}

/* Le um arquivo inteiro para a memoria. Devolve NULL se nao achar. Quem chama deve dar free(). */
static char *lerArquivo(const char *caminho)
{
    FILE *arq = fopen(caminho, "rb");
    if (arq == NULL)
    {
        return NULL;
    }
    fseek(arq, 0, SEEK_END);
    long tamanho = ftell(arq);
    rewind(arq);

    char *conteudo = (char *)malloc(tamanho + 1);
    if (conteudo == NULL)
    {
        fclose(arq);
        return NULL;
    }
    size_t lidos = fread(conteudo, 1, tamanho, arq);
    conteudo[lidos] = '\0';
    fclose(arq);
    return conteudo;
}

/* Solicita usuario e senha (maximo de 3 tentativas).
 * Retorna 1 se o login for valido ("admin" / "1234") ou 0 se esgotar as tentativas. */
int fazerLogin(void)
{
    printf("========== LOGIN ==========\n");
    char usuario[50], senha[50];
    for (tentativa = 3; tentativa >= 1; tentativa--)
    {
        printf("Digite o usuario: \n");
        scanf("%s", &usuario);
        printf("Digite a senha: \n");
        scanf("%s", &senha);
        /* pedir usu�rio e senha com capturarEntrada(...) */
        if (strcmp(usuario, "admin") == 0 && strcmp(senha, "1234") == 0)
        {
            printf("Login realizado com sucesso! Bem-vindo, %s.\n", usuario);
            return 1;
        }
        else
        {
            printf("Usuario ou senha incorretos. Tentativas restantes: %i\n", tentativa - 1);
        }
        /* se errou: mostrar quantas tentativas restam (3 - tentativa) */
    }
    printf("Tentativas esgotadas");
    /* chegou aqui? errou as 3 vezes: avisar e return 0; */
    return 0;
}

fazerLogin();


/* ===================== Funcoes da impressora ===================== */

/* Solicita ao usuario os parametros de comunicacao (tipo, modelo, porta e parametro extra)
 * e salva nas variaveis globais, caso a conexao ainda nao esteja aberta. */
void configurarConexao(void)
{
    if (!conexaoAberta)
    {
        tipo = lerInteiro("Digite o tipo de conexao (1 = USB, 2 = Serial, 3 = Rede (IP), 4 = Bluetooth): ");
        capturarEntrada("Digite o modelo da impressora (i8 ou i9): ", modelo, sizeof(modelo));
        capturarEntrada("Digite a porta de conexao (ex: USB): ", conexao, sizeof(conexao));
        parametro = lerInteiro("Digite o parametro adicional (ex: 0 para padrao): ");
        printf("Parametros de conexao configurados com sucesso.\n");
    }
    else
    {
        printf("Conexao ja configurada. Pronta para uso.\n");
    }
}

/* Tenta estabelecer a comunicacao com a impressora usando os parametros configurados.
 * Atualiza a flag global 'conexaoAberta' para 1 se a DLL retornar 0 (sucesso). */
void abrirConexao(void)
{
    if (!conexaoAberta)
    {
        int retorno = AbreConexaoImpressora(tipo, modelo, conexao, parametro);
        if (retorno == 0)
        {
            conexaoAberta = 1;
            printf("Conexao aberta com sucesso.\n");
        }
        else
        {
            printf("Erro ao abrir conexao. Codigo de erro: %d\n", retorno);
        }
    }
    else
    {
        printf("Conexao ja esta aberta.\n");
    }
}

/* Encerra a comunicacao ativa com a impressora e redefine 'conexaoAberta' para 0. */
void fecharConexao(void)
{
    int retorno = FechaConexaoImpressora();
    conexaoAberta = 0;
}

/* Verifica se ha conexao ativa, le um texto e a quantidade de copias desejada,
 * imprime cada copia avancando 1 linha e realiza o corte do papel ao final. */
void imprimirTexto(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    char texto[200]; /* 2) s� agora pergunta */
    capturarEntrada("Texto a imprimir: ", texto, sizeof(texto));
    int copias = lerInteiro("Quantas copias? ");
    int i;
    for (i = 0; i <= copias; i++)
    {                                                 /* la�o: repete a impress�o */
        int retorno = ImpressaoTexto(texto, 1, 4, 0); /* 3) chama o comando */
        AvancaPapel(1);
        if (retorno >= 0)
        { /* 4) confere o retorno */
            printf("Copia %d impressa!\n", i);
        }
        else
        {
            printf("Erro ao imprimir. Codigo: %d\n", retorno);
        }
    }
    Corte(3); /* corta o papel */
}

/* Le uma string do usuario e a envia para impressao em formato QR Code (tamanho 6, correcao 4),
 * acionando a guilhotina (corte) logo em seguida. */
void imprimirQRCode(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    char dados[200];
    capturarEntrada("Conteudo do QrCode: ", dados, sizeof(dados));
    int retorno = ImpressaoQRCode(dados, 6, 4);
    Corte(3);
}

/* Imprime um codigo de barras padrao (tipo 8 / CODE128) com valor fixo e realiza o corte do papel. */
void imprimirCodigoBarras(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    int retorno = ImpressaoCodigoBarras(8, "{A012345678912", 100, 2, 3);
    Corte(3);
}

/* Carrega o arquivo 'XMLSAT.xml' para a memoria, envia o conteudo para impressao fiscal SAT,
 * libera a memoria alocada (free) e executa o corte do papel. */
void imprimirXMLSAT(void)
{
    if (!conexaoAberta)
    {
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return;
    }
    char *dadosXML = lerArquivo("XMLSAT.xml");
    if (dadosXML == NULL)
    { /* arquivo n�o encontrado */
        printf("Nao foi possivel ler o XMLSAT.xml\n");
        return;
    }
    int retorno = ImprimeXMLSAT(dadosXML, 0);
    Corte(3);
    free(dadosXML); /* devolve a mem�ria */
    if (retorno == 0)
    {
        printf("XML SAT impresso!\n");
    }
    else
    {
        printf("Erro ao imprimir XML SAT. Codigo: %d\n", retorno);

        /* le o arquivo XML (na raiz do projeto) para a memoria */
        // char *dadosXML = lerArquivo("XMLSAT.xml");
    }
}

/* Carrega o arquivo 'CANC_SAT.xml' e imprime o comprovante de cancelamento SAT
 * utilizando a chave de assinatura 'ASS_QRCODE', finalizando com o corte do papel. */
void imprimirXMLCancelamentoSAT(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    char *dadosXML = lerArquivo("CANC_SAT.xml");
    int retorno = ImprimeXMLCancelamentoSAT(dadosXML, ASS_QRCODE, 0);

    /* le o arquivo XML (na raiz do projeto) para a memoria */
    // char *dadosXML = lerArquivo("CANC_SAT.xml");
    Corte(3);
}

/* Envia um pulso eletrico padrao Elgin (pino 1, 50ms) para abertura de gaveta de dinheiro. */
void abrirGavetaElgin(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    int retorno = AbreGavetaElgin(1, 50, 50);
}

/* Envia um pulso eletrico generico (pino 1, ti=5, tf=10) para abertura de gaveta de dinheiro. */
void abrirGaveta(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    int retorno = AbreGaveta(1, 5, 10);
}

/* Aciona o buzzer interno da impressora emitindo 2 bipes curtos. */
void emitirSinalSonoro(void)
{
    if (!conexaoAberta)
    { /* 1) conex�o aberta? */
        printf("Conexao fechada. Abra a conexao antes (opcao 2).\n");
        return; /* n�o est�: avisa e SAI da fun��o */
    }
    int retorno = SinalSonoro(2, 50, 5);
}

/* Exibe no console a lista numerada com todas as operacoes disponiveis no sistema. */
void exibirMenu(void)
{
    printf("\n*************************************************\n");
    printf("**************** MENU IMPRESSORA ****************\n");
    printf("*************************************************\n\n");

    printf("1  - Configurar Conexao\n");
    printf("--------------------------------------\n");
    printf("2  - Abrir conexao\n");
    printf("--------------------------------------\n");
    printf("3  - Fechar conexao\n");
    printf("--------------------------------------\n");
    printf("4  - Imprimir texto\n");
    printf("--------------------------------------\n");
    printf("5  - Imprimir QR Code\n");
    printf("--------------------------------------\n");
    printf("6  - Imprimir codigo de barras\n");
    printf("--------------------------------------\n");
    printf("7  - Imprimir XML SAT\n");
    printf("--------------------------------------\n");
    printf("8  - Imprimir XML de cancelamento SAT\n");
    printf("--------------------------------------\n");
    printf("9  - Abrir gaveta (Elgin)\n");
    printf("--------------------------------------\n");
    printf("10  - Abrir gaveta\n");
    printf("--------------------------------------\n");
    printf("11  - Sinal sonoro\n");
    printf("--------------------------------------\n");
    printf("0  - Sair\n");
    printf("--------------------------------------\n");
}

/* ===================== Programa principal ===================== */

int main(void)
{
    if (!carregarDLL())
    {
        printf("Programa encerrado.\n");
        return 1;
    }

    if (!fazerLogin())
    { /* depois do carregarDLL() */
        printf("Acesso negado.\n");
        FreeLibrary(dll);
        return 1;
    }

    int opcao = -1;
    while (opcao != 0)
    {
        exibirMenu();
        opcao = lerInteiro("\nDigite a opcao desejada: ");

        switch (opcao)
        {
        case 1:
            configurarConexao();
            break;

        case 2:
            abrirConexao();
            break;

        case 3:
            fecharConexao();
            break;

        case 4:
            imprimirTexto();
            break;

        case 5:
            imprimirQRCode();
            break;

        case 6:
            imprimirCodigoBarras();
            break;

        case 7:
            imprimirXMLSAT();
            break;

        case 8:
            imprimirXMLCancelamentoSAT();
            break;

        case 9:
            abrirGavetaElgin();
            break;

        case 10:
            abrirGaveta();
            break;

        case 11:
            emitirSinalSonoro();
            break;

        case 0:
            printf("Saindo...");
            // Sair();
            fecharConexao();
            break;
        }

        //...
    }

    FreeLibrary(dll);
    return 0;
}
