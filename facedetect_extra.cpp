#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>
#include <deque>
#include <string>
#include <locale>
#include <bits/stdc++.h>
#include <cstdlib>  

using namespace std;
using namespace cv;
using namespace std::chrono;

int recorde = 0; // Variável para armazenar o recorde
bool jaSalvo = false;
int blockSize= 20;  // estava 20 o tamanho;
int pontuacao = 0;
Mat cobrinha;     // imagem da cobrinha
Mat background;   // imagem do plano de fundo
Mat comidaImage;  //imagem do ovo
Mat comidaImage2; // imagem da comida 2
Mat chaser;  // Imagem do sasuke
char key = 0;
int tempoLimite = 10;

// Posição da "cobrinha" e da comida
Point snakePos(100, 100);
Point comida; //
Point comida2; // posição da comida
Point chaserPos(20, 20); // Posição inicial do perseguidor
int chaserSpeed = 1; // Velocidade do perseguidor

// Variável para armazenar o tempo inicial do jogo
steady_clock::time_point startTime;

// Função para salvar a pontuação
class Pontuacao {
private:
    int maiorPontuacao;
    string nomeArquivo;

public:
    // Construtor da classe que recebe o nome do arquivo como parâmetro
    Pontuacao(const string& arquivo) : nomeArquivo(arquivo), maiorPontuacao(0) {
        // Carrega a maior pontuação do arquivo, se ele existir
        ifstream file(nomeArquivo);
        if (file.is_open()) {
            file >> maiorPontuacao;  // Lê a maior pontuação do arquivo
            file.close();
        }
    }

    // Função para verificar e atualizar a pontuação
    void verificarPontuacao(int pontuacaoAtual) {
        if (pontuacaoAtual > maiorPontuacao) {
            maiorPontuacao = pontuacaoAtual;
            salvarPontuacao();  // Salva a nova pontuação no arquivo
            cout << "Nova maior pontuação salva: " << maiorPontuacao << endl;
        } else {
            cout << "A pontuação atual não é maior que a salva: " << maiorPontuacao << endl;
        }
    }

    // Função para salvar a maior pontuação no arquivo
    void salvarPontuacao() {
        ofstream file(nomeArquivo);
        if (file.is_open()) {
            file << maiorPontuacao;  // Salva a nova maior pontuação
            file.close();
        } else {
            cout << "Erro ao abrir o arquivo para salvar a pontuação." << endl;
        }
    }

    // Função para retornar a maior pontuação
    int getMaiorPontuacao() const {
        return maiorPontuacao;
    }
};

class Som {
private:
    string caminhoDoArquivo;

public:
    // Construtor que recebe o caminho do arquivo de som
    Som(const string& caminho) : caminhoDoArquivo(caminho) {}

    // Método para tocar o som
    void tocar() const {
        string comando = "mpg123 \"" + caminhoDoArquivo + "\"";
        system(comando.c_str());
    }

    // Método para mudar o arquivo de som
    void setCaminhoDoArquivo(const string& caminho) {
        caminhoDoArquivo = caminho;
    }

    // Método para obter o caminho atual do arquivo de som
    string getCaminhoDoArquivo() const {
        return caminhoDoArquivo;
    }
};

// Função para tocar o efeito sonoro
/*void tocarEfeitoSonoro() {
    // Caminho para o arquivo .mp3 com espaços
    string caminhoDoArquivo = "./Efeito_Chidori.mp3";

    // Executa o mpg123 para tocar o som, incluindo o caminho entre aspas
    string comando = "mpg123 \"" + caminhoDoArquivo + "\"";
    system(comando.c_str());
}
void EfeitoSonoro2() {
    // Caminho para o arquivo .mp3 com espaços
    string caminhoDoArquivo2 = "./Comer.mp3";

    // Executa o mpg123 para tocar o som, incluindo o caminho entre aspas
    string comando = "mpg123 \"" + caminhoDoArquivo2 + "\"";
    system(comando.c_str());
}
*/
// Função para carregar o recorde do arquivo
void carregarRecorde(const string& arquivo) {
    ifstream inputFile(arquivo);
    if (inputFile.is_open()) {
        string linha;
        if (getline(inputFile, linha)) {
            recorde = stoi(linha); // Lê a pontuação do arquivo
        }
        inputFile.close();
    }
}

// Função para salvar o recorde no arquivo
void salvarRecorde(const string& arquivo) {
    ofstream outputFile(arquivo);
    if (outputFile.is_open()) {
        outputFile << recorde; // Salva o recorde no arquivo
        outputFile.close();
    }
}

// Função para reiniciar o jogo
void ReiniciarGame() {
    pontuacao = 0;
    jaSalvo = false;
    comida = Point(rand() % 30 * blockSize, rand() % 20 * blockSize);  // Gera a posição da primeira comida
    comida2 = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera a posição da segunda comida
    chaserPos = Point(20, 20); // Reinicia a posição do perseguidor
    // Reinicia o tempo do jogo
    startTime = steady_clock::now();
}

// Desenha a comida com uma imagem PNG
void drawFood(Mat& frame) {
    // Desenha a primeira comida
    if (!comidaImage.empty()) {
        Mat mask, comidaResized;
        int foodSize = blockSize * 3;
        resize(comidaImage, comidaResized, Size(foodSize, foodSize));
        vector<Mat> layers;
        split(comidaResized, layers);
        if (layers.size() == 4) {
            Mat rgb[3] = {layers[0], layers[1], layers[2]};
            mask = layers[3];
            merge(rgb, 3, comidaResized);
            comidaResized.copyTo(frame(Rect(comida.x, comida.y, comidaResized.cols, comidaResized.rows)), mask);
        } else {
            comidaResized.copyTo(frame(Rect(comida.x, comida.y, comidaResized.cols, comidaResized.rows)));
        }
    }

    // Desenha a segunda comida
    if (!comidaImage2.empty()) {
        Mat mask, comidaResized2;
        int foodSize2 = blockSize * 3;
        resize(comidaImage2, comidaResized2, Size(foodSize2, foodSize2));
        vector<Mat> layers2;
        split(comidaResized2, layers2);
        if (layers2.size() == 4) {
            Mat rgb2[3] = {layers2[0], layers2[1], layers2[2]};
            mask = layers2[3];
            merge(rgb2, 3, comidaResized2);
            comidaResized2.copyTo(frame(Rect(comida2.x, comida2.y, comidaResized2.cols, comidaResized2.rows)), mask);
        } else {
            comidaResized2.copyTo(frame(Rect(comida2.x, comida2.y, comidaResized2.cols, comidaResized2.rows)));
        }
    }
}

// Função para desenhar a imagem da "cobra"
void drawSnake(Mat& frame) {
    if (!cobrinha.empty()) {
        Mat mask, snakeResized;
        int snakeSize = blockSize * 4 ; // Tamanho constante da cobra
        resize(cobrinha, snakeResized, Size(snakeSize, snakeSize)); // Ajusta o tamanho da cobra
        vector<Mat> layers;
        split(snakeResized, layers);  // Separa os canais de cor
        if (layers.size() == 4) {     // Imagem com transparência
            Mat rgb[3] = {layers[0], layers[1], layers[2]};
            mask = layers[3];         // Canal alfa (transparência)
            merge(rgb, 3, snakeResized);  // Junta os canais RGB
            snakeResized.copyTo(frame(Rect(snakePos.x, snakePos.y, snakeResized.cols, snakeResized.rows)), mask);
            //resize(cobrinha, cobrinha, Size(200, 200));
        } else {
            snakeResized.copyTo(frame(Rect(snakePos.x, snakePos.y, snakeResized.cols, snakeResized.rows)));
            
        }
    }
}


    // Função para desenhar o perseguidor com imagem PNG
void drawChaser(Mat& frame) {
    if (!chaser.empty()) {
        Mat mask, chaserResized;
        int chaserSize = blockSize * 4;  // Ajuste o tamanho conforme necessário
        resize(chaser, chaserResized, Size(chaserSize, chaserSize));
        vector<Mat> layers;
        split(chaserResized, layers);

        // Verifica se a imagem possui um canal alfa (transparência)
        if (layers.size() == 4) {
            Mat rgb[3] = {layers[0], layers[1], layers[2]};
            mask = layers[3];  // Canal alfa
            merge(rgb, 3, chaserResized);
            chaserResized.copyTo(frame(Rect(chaserPos.x, chaserPos.y, chaserResized.cols, chaserResized.rows)), mask);
        } else {
            chaserResized.copyTo(frame(Rect(chaserPos.x, chaserPos.y, chaserResized.cols, chaserResized.rows)));
        }
    }
}

void exibirMenuInicial(Mat& background) {
    Mat menuFrame = background.clone();
    
    // Exibe o nome do jogo "Salve o Itachi"
    putText(menuFrame, "Salve o Itachi", Point(100, 150), FONT_HERSHEY_SIMPLEX, 2, Scalar(255, 255, 255), 3);
    
    // Exibe a instrução para iniciar o jogo
    putText(menuFrame, "Pressione 's' para iniciar", Point(100, 300), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
    
    // Mostra o menu na janela
    imshow("Snake Game", menuFrame);

    // Espera até que o jogador pressione 's' para iniciar o jogo
    char key;
    do {
        key = (char)waitKey(0);  // Espera indefinidamente por uma tecla
    } while (key != 's');  // Continua esperando até o jogador pressionar 's'
}

// Função para exibir o resultado final (pontuação e recorde)
void exibirResultadoFinal(Mat& frame, int pontuacaoAtual, int recorde) {
    // Exibe "Game Over"
    putText(frame, "Game Over!!", Point(150, 240), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(255, 255, 255), 3);
    
    // Exibe a pontuação atual
    putText(frame, "Pontuacao: " + to_string(pontuacaoAtual), Point(150, 300), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
    
    // Verifica se é um novo recorde
    if (pontuacaoAtual > recorde) {
        putText(frame, "Novo Recorde!!", Point(150, 350), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
    } else {
        putText(frame, "Recorde: " + to_string(recorde), Point(150, 350), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
    }

    // Mostra a tela de resultado final
    imshow("Snake Game", frame);
    waitKey(3000);  // Espera 3 segundos para exibir o resultado
}


// Atualiza a posição da cobra com base na detecção do rosto
void updateGame(Point faceCenter) {
    // Atualiza a posição da cabeça da cobra
    snakePos = Point(faceCenter.x - blockSize, faceCenter.y - blockSize );
    Som somComer("./Comer.mp3");

    // Verifica se comeu a primeira comida
    if (abs(snakePos.x - comida.x) < blockSize * 1 && abs(snakePos.y - comida.y) < blockSize * 1) {
        pontuacao += 10;
        somComer.tocar();

        comida = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera nova posição para a primeira comida
    }

    // Verifica se comeu a segunda comida
    if (abs(snakePos.x - comida2.x) < blockSize * 1 && abs(snakePos.y - comida2.y) < blockSize * 1) {
        pontuacao += 10;
        somComer.tocar();

        comida2 = Point(rand() % 30 * blockSize, rand() % 20 * blockSize); // Gera nova posição para a segunda comida
    }
}

// Atualiza a posição do perseguidor
bool updateChaser() {
    // Move o perseguidor em direção à cobra
    if (chaserPos.x < snakePos.x) {
        chaserPos.x += chaserSpeed; // Move para a direita
    } else if (chaserPos.x > snakePos.x) {
        chaserPos.x -= chaserSpeed; // Move para a esquerda
    }

    if (chaserPos.y < snakePos.y) {
        chaserPos.y += chaserSpeed; // Move para baixo
    } else if (chaserPos.y > snakePos.y) {
        chaserPos.y -= chaserSpeed; // Move para cima
    }
  Mat displayFrame = background.clone();
    // Verifica se o perseguidor alcançou a cobrinha  
           if (abs(chaserPos.x - snakePos.x) < blockSize * 1.5 && abs(chaserPos.y - snakePos.y) < blockSize * 1.5 ) {
            putText(displayFrame, " Game Over!! ", Point(150, 240), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(255, 255, 255), 3);
            
    Som efeitoChidori("./Efeito_Chidori.mp3");
        
         efeitoChidori.tocar();

         
         cout << "Game Over! O Sasuke alcançou o Itachi." << endl;
        Pontuacao pontuacaoManager("tabela_de_pontuacoes.txt2");
         pontuacaoManager.verificarPontuacao(pontuacao); // Salva a pontuação
        return true; 
    }return false;

}

int main(int argc, const char** argv) {
    srand(time(0));
    VideoCapture capture;
    Mat frame;
    CascadeClassifier faceCascade;
    string faceCascadeName = "haarcascade_frontalface_default.xml";
    string arquivoRecorde = "tabela_de_pontuacoes.txt"; // Nome do arquivo para salvar o recorde

    
 // Carrega o recorde do arquivo
    carregarRecorde(arquivoRecorde);

    
        Pontuacao pontuacaoManager(arquivoRecorde);

    // Inicializa o tempo do jogo
    startTime = steady_clock::now();
    //inicializa o arquivo
    // Pontuacao pontuacaoManager("tabela_de_pontuacoes.txt2");

    // Carrega a imagem do perseguidor
    chaser = imread("chaser.png", IMREAD_UNCHANGED);
    if (chaser.empty()) {
    cout << "ERROR: Could not load chaser image." << endl;
    return -1;
    }

    // Carrega o Haar Cascade para detecção de rostos
    if (!faceCascade.load(faceCascadeName)) {
        cout << "ERROR: Could not load classifier cascade: " << faceCascadeName << endl;
        return -1;
    }

    // Carrega a imagem que será usada como "cobra"
    cobrinha = imread("cobrinha.png", IMREAD_UNCHANGED);
    if (cobrinha.empty()) {
        cout << "ERROR: Could not load snake image." << endl;
        return -1;
    }

    // Carrega a imagem de comida
    comidaImage = imread("comida.png", IMREAD_UNCHANGED); 
    if (comidaImage.empty()) {
        cout << "ERROR: Could not load food image." << endl;
        return -1;
    }

    comidaImage2 = imread("comida2.png", IMREAD_UNCHANGED);  
    if (comidaImage2.empty()) {
        cout << "ERROR: Could not load second food image." << endl;
        return -1;
    }

    // Carrega a imagem de plano de fundo
    background = imread("background.jpg", IMREAD_COLOR);
    if (background.empty()) {
        cout << "ERROR: Could not load background image." << endl;
        return -1;
    }
    int windowWidth = 640;  // Altere conforme necessário
    int windowHeight = 480; // Altere conforme necessário
    resize(background, background, Size(windowWidth, windowHeight));  // Redimensiona o plano de fundo

    
    if (!capture.open(0)) {
        cout << "Capture from camera #0 didn't work" << endl;
        return 1;
    }
    //capture.set(CAP_PROP_FRAME_WIDTH, windowWidth);  // Define a largura da captura de vídeo
    //capture.set(CAP_PROP_FRAME_HEIGHT, windowHeight); // Define a altura da captura de vídeo

    namedWindow("Snake Game", WINDOW_NORMAL);

    ReiniciarGame();

    exibirMenuInicial(background);


    while (true) {
        capture >> frame;
        if (frame.empty())
            break;

        if (key == 0) // just first time
            resizeWindow("Snake Game", frame.cols, frame.rows);

        flip(frame, frame, 1);

        Mat grayFrame;
        cvtColor(frame, grayFrame, COLOR_BGR2GRAY);
        vector<Rect> faces;

        // Detecta rostos na imagem
        faceCascade.detectMultiScale(grayFrame, faces, 1.1, 2, 0 | CASCADE_SCALE_IMAGE, Size(100, 100));

        if (!faces.empty()) {
            // Usa o centro do primeiro rosto detectado como a posição da cobra
            Point faceCenter(faces[0].x + faces[0].width /4, faces[0].y + faces[0].height/ 4);
            updateGame(faceCenter);
        }

        // Atualiza a posição do perseguidor
        //updateChaser();

        // Copia a imagem de fundo para exibição
        Mat displayFrame = background.clone();
        //Mat displayFrame = background;

        // Desenha a "cobra" (imagem da cabeça), a comida e o perseguidor no plano de fundo
        drawSnake(displayFrame);
        drawFood(displayFrame);
        drawChaser(displayFrame);

        // Exibe o placar
        putText(displayFrame, "Placar: " + to_string(pontuacao), Point(430,470 ), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
        // Calcula o tempo de jogo
        steady_clock::time_point currentTime = steady_clock::now();
        duration<double> elapsedTime = duration_cast<duration<double>>(currentTime - startTime);

        // Calcula o tempo restante
        int tempoRestante = tempoLimite - (int)elapsedTime.count();

        // Exibe o tempo de jogo decorrido
        putText(displayFrame, "Tempo: " + to_string(tempoRestante) + "s", Point(30, 470), FONT_HERSHEY_SIMPLEX, 1, Scalar(255, 255, 255), 2);
       // Verifica se o tempo limite foi atingido
        if (tempoRestante <= 0) {
            cout << "Tempo limite atingido! Game Over!!" << endl;
            exibirResultadoFinal(displayFrame, pontuacao, recorde);  // Exibe o resultado final
            pontuacaoManager.verificarPontuacao(pontuacao);  // Salva a pontuação
            ReiniciarGame();
            exibirMenuInicial(background);  // Retorna ao menu inicial
            continue;  // Reinicia o jogo
        }
       /* if (updateChaser()) {
            // Copia a imagem de fundo para exibição
            Mat displayFrame = background.clone();
            // Desenha a "cobra" (imagem da cabeça), a comida e o perseguidor no plano de fundo
            drawSnake(displayFrame);
            drawFood(displayFrame);
            drawChaser(displayFrame);
            // Exibe a mensagem de Game Over
            putText(displayFrame, " Game Over!! ", Point(150, 240), FONT_HERSHEY_SIMPLEX, 1.5, Scalar(255, 255, 255), 3);
            imshow("Snake Game", displayFrame);
            waitKey(3000);  // Exibe a mensagem por 3 segundos
              pontuacaoManager.verificarPontuacao(pontuacao);  // Salva a pontuação
              // Reinicia o jogo
            ReiniciarGame();

            // Chama o menu inicial novamente
            exibirMenuInicial(background);
            continue;  // Retorna ao início do loop para reiniciar o jogo
        }
        */
         // Copia a imagem de fundo para exibição
        //Mat displayFrame = background.clone();
        
        
             // Verifica se o perseguidor alcançou a cobrinha
        if (updateChaser()) {
            exibirResultadoFinal(displayFrame, pontuacao, recorde);  // Exibe o resultado final
            pontuacaoManager.verificarPontuacao(pontuacao);  // Salva a pontuação
            ReiniciarGame();
            exibirMenuInicial(background);  // Retorna ao menu inicial
            continue;  // Reinicia o jogo
        }
            // Exibe a mensagem de Game Over
            
        // Desenha retângulos ao redor dos rostos detectados
        for (size_t i = 0; i < faces.size(); i++) {
            Rect r = faces[i];

            rectangle(frame, Point(cvRound(r.x), cvRound(r.y)),
                      Point(cvRound((r.x + (r.width/1.5) - 1)), cvRound((r.y + (r.height/1.5)- 1))),
                      Scalar(255, 0, 0), 3); // Desenha um retângulo azul
        }

        // Exibe o frame com a cobra, comida e perseguidor
        imshow("Snake Game", displayFrame);

        key = (char)waitKey(30);
        if (key == 27) {  // ESC para sair
             pontuacaoManager.verificarPontuacao(pontuacao);
            break;
        }
    }

    return 0;
}
