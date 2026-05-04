#include "raylib.h"
#include <stdio.h>

#define MAX_BORU 100
#define BORU_GENISLIK 340
#define BORU_ARALIK 300 // Borular arasındaki yatay mesafe
#define MAX_KAR 100 // Maksimum kar tanesi sayısı

// --- HITBOX İNCE AYAR (Buradan manuel değiştirin) ---
#define HB_X_KAYDIRMA 150.0f  // Kutuyu sağa (+) veya sola (-) kaydır
#define HB_Y_KAYDIRMA 10.0f   // Kutuyu aşağı (+) veya yukarı (-) kaydır
#define HB_GENISLIK_DARALT 300.0f // Kutuyu ne kadar daraltalım? (Toplam)
#define HB_YUKSEKLIK_DARALT 20.0f  // Kutuyu ne kadar kısaltalım? (Toplam)
// ----------------------------------------------------

typedef struct Kus {
    Vector2 pozisyon;
    float yaricap;
    float hiz;
    float ziplamaGucu;
    Color renk;
} Kus;

typedef struct Boru {
    Rectangle ust;
    Rectangle alt;
    bool aktif;
    bool gecildi;
} Boru;

typedef struct KarTanesi {
    Vector2 pozisyon;
    float hiz;
    float cap;
} KarTanesi;

// Global değişkenler
const int ekranGenislik = 800;
const int ekranYukseklik = 450;
const float yerCekimi = 0.5f; // Yerçekimi kuvveti
Kus kus;
Boru borular[MAX_BORU];
KarTanesi karlar[MAX_KAR];
int skor = 0;
int enYuksekSkor = 0;
bool oyunBitti = false;
Texture2D arkaplan; // Potansiyel doku için yer tutucu

// Fonksiyon bildirimleri
void OyunuBaslat();
void OyunuGuncelle();
void OyunuCiz();
void OyunuKapat();
void SkoruYukle();
void SkoruKaydet();

// Ses Değişkenleri
Sound ziplamaSesi;
Music arkaPlanMuzigi;
bool muzikCaliyor = false;

// Video (GIF) Değişkenleri
Image noelBabaResim;
Texture2D noelBabaDoku;
Texture2D boruDoku; // Boru dokusu
int animKareSayisi = 0;
int suankiKare = 0;
int kareSayaci = 0;
int kareGecikmesi = 8; // Animasyon hızı
unsigned int sonrakiKareOfseti = 0;

int main(void) {
    InitWindow(ekranGenislik, ekranYukseklik, "Flappy Bird - Raylib");
    InitAudioDevice(); // Ses aygıtını başlat

    // Ses ve Müzik Yükleme
    ziplamaSesi = LoadSound("voice/jump.mp3");
    SetSoundVolume(ziplamaSesi, 0.4f); // Zıplama sesi seviyesini kıs
    arkaPlanMuzigi = LoadMusicStream("voice/oyun boyu arkada dönecek müzik.mp3");
    
    // GIF Yükleme
    noelBabaResim = LoadImageAnim("voice/santa_video.gif", &animKareSayisi);
    noelBabaDoku = LoadTextureFromImage(noelBabaResim);

    // Arkaplan Yükleme
    arkaplan = LoadTexture("background2.png");
    
    // Boru Yükleme
    boruDoku = LoadTexture("boru 3.png");

    SetMusicVolume(arkaPlanMuzigi, 0.5f); // Müziğin ses seviyesi
    PlayMusicStream(arkaPlanMuzigi);

    SetTargetFPS(60);

    SkoruYukle(); // Başlangıçta en yüksek skoru yükle
    OyunuBaslat();
    
    while (!WindowShouldClose()) {
        UpdateMusicStream(arkaPlanMuzigi); // Müzik akışını güncelle
        
        if (!oyunBitti) {
            OyunuGuncelle();
        } else {
            if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                OyunuBaslat();
            }
            
            // GIF Animasyon Güncelleme
            kareSayaci++;
            if (kareSayaci >= kareGecikmesi) {
                suankiKare++;
                if (suankiKare >= animKareSayisi) suankiKare = 0;

                sonrakiKareOfseti = noelBabaResim.width * noelBabaResim.height * 4 * suankiKare;
                UpdateTexture(noelBabaDoku, ((unsigned char *)noelBabaResim.data) + sonrakiKareOfseti);
                kareSayaci = 0;
            }
        }
        OyunuCiz();
    }

    UnloadSound(ziplamaSesi);
    UnloadMusicStream(arkaPlanMuzigi);
    UnloadImage(noelBabaResim);  // Resmi hafızadan sil
    UnloadTexture(noelBabaDoku); // Dokuyu hafızadan sil
    UnloadTexture(arkaplan);     // Arkaplanı sil
    UnloadTexture(boruDoku);     // Boru dokusunu sil
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

void OyunuBaslat() {
    kus.pozisyon = (Vector2){ 80, ekranYukseklik / 2.0f };
    kus.yaricap = 25;
    kus.hiz = 0;
    kus.ziplamaGucu = 8.0f;
    kus.renk = RED; // Noel Baba kuşu

    for (int i = 0; i < MAX_BORU; i++) {
        borular[i].aktif = false;
        borular[i].gecildi = false;
    }

    // İlk birkaç boruyu başlat
    for (int i = 0; i < 5; i++) {
        float posX = ekranGenislik + i * BORU_ARALIK;
        float boslukY = GetRandomValue(50, ekranYukseklik - 150);
        float boslukYukseklik = 160;
        
        borular[i].ust = (Rectangle){ posX, 0, BORU_GENISLIK, boslukY };
        borular[i].alt = (Rectangle){ posX, boslukY + boslukYukseklik, BORU_GENISLIK, ekranYukseklik - (boslukY + boslukYukseklik) };
        borular[i].aktif = true;
    }

    // Kar tanelerini başlat
    for (int i = 0; i < MAX_KAR; i++) {
        karlar[i].pozisyon = (Vector2){ (float)GetRandomValue(0, ekranGenislik), (float)GetRandomValue(0, ekranYukseklik) };
        karlar[i].hiz = (float)GetRandomValue(100, 300) / 100.0f; // 1.0 ile 3.0 arasında hız
        karlar[i].cap = (float)GetRandomValue(2, 5);
    }

    skor = 0;
    oyunBitti = false;
}

void OyunuGuncelle() {
    // Kuş Hareketi
    if (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        kus.hiz = -kus.ziplamaGucu;
        PlaySound(ziplamaSesi); // Zıplama sesi
    }

    kus.hiz += yerCekimi;
    kus.pozisyon.y += kus.hiz;

    // Sınırları Kontrol Et
    if (kus.pozisyon.y >= ekranYukseklik - kus.yaricap) {
        kus.pozisyon.y = ekranYukseklik - kus.yaricap;
        oyunBitti = true;
        if (skor > enYuksekSkor) {
            enYuksekSkor = skor;
            SkoruKaydet();
        }
        // OpenURL kaldırıldı, GIF oyun içinde oynayacak
    } else if (kus.pozisyon.y <= kus.yaricap) {
        kus.pozisyon.y = kus.yaricap;
        kus.hiz = 0;
    }

    // Boru Hareketi ve Çarpışma
    for (int i = 0; i < MAX_BORU; i++) {
        if (borular[i].aktif) {
            borular[i].ust.x -= 3; // Hareket hızı
            borular[i].alt.x -= 3;

            // Çarpışma kontrolü (MANUEL AYARLI)
            Rectangle ustHitbox = (Rectangle){ 
                borular[i].ust.x + HB_X_KAYDIRMA, 
                borular[i].ust.y + HB_Y_KAYDIRMA, 
                borular[i].ust.width - HB_GENISLIK_DARALT, 
                borular[i].ust.height - HB_YUKSEKLIK_DARALT 
            };
            
            Rectangle altHitbox = (Rectangle){ 
                borular[i].alt.x + HB_X_KAYDIRMA, 
                borular[i].alt.y + HB_Y_KAYDIRMA, 
                borular[i].alt.width - HB_GENISLIK_DARALT, 
                borular[i].alt.height - HB_YUKSEKLIK_DARALT 
            };

            if (CheckCollisionCircleRec(kus.pozisyon, kus.yaricap, ustHitbox) ||
                CheckCollisionCircleRec(kus.pozisyon, kus.yaricap, altHitbox)) {
                oyunBitti = true;
                // PlaySound(carpmaSesi); // Ses dosyası varsa buraya eklenebilir
                if (skor > enYuksekSkor) {
                    enYuksekSkor = skor;
                    SkoruKaydet();
                }
                // OpenURL kaldırıldı
            }

            // Boruyu geri dönüştür
            if (borular[i].ust.x + BORU_GENISLIK < 0) {
                // Bu boruyu en sağdaki borunun arkasına yerleştirmek için en sağdaki boruyu bul
                float maxX = 0;
                for (int j = 0; j < MAX_BORU; j++) {
                    if (borular[j].aktif && borular[j].ust.x > maxX) {
                        maxX = borular[j].ust.x;
                    }
                }

                float posX = maxX + BORU_ARALIK;
                float boslukY = GetRandomValue(50, ekranYukseklik - 150);
                float boslukYukseklik = 160;

                borular[i].ust = (Rectangle){ posX, 0, BORU_GENISLIK, boslukY };
                borular[i].alt = (Rectangle){ posX, boslukY + boslukYukseklik, BORU_GENISLIK, ekranYukseklik - (boslukY + boslukYukseklik) };
                borular[i].gecildi = false;
            }

            // Skor güncelleme
            if (!borular[i].gecildi && borular[i].ust.x + BORU_GENISLIK < kus.pozisyon.x) {
                skor++;
                borular[i].gecildi = true;
            }
        }
    }
    
    if (oyunBitti) {
        if (skor > enYuksekSkor) enYuksekSkor = skor;
    }

    // Kar Yağışı Güncelleme
    for (int i = 0; i < MAX_KAR; i++) {
        karlar[i].pozisyon.y += karlar[i].hiz;

        if (karlar[i].pozisyon.y > ekranYukseklik) {
            karlar[i].pozisyon.y = -10;
            karlar[i].pozisyon.x = (float)GetRandomValue(0, ekranGenislik);
        }
    }
}

void OyunuCiz() {
    BeginDrawing();
    
    if (!oyunBitti) {
        // Oyun oynanırken resimli arkaplan
        DrawTexturePro(arkaplan, 
            (Rectangle){ 0, 0, arkaplan.width, arkaplan.height }, 
            (Rectangle){ 0, 0, ekranGenislik, ekranYukseklik }, 
            (Vector2){ 0, 0 }, 
            0.0f, WHITE);
            
        // Boruları Çiz
        for (int i = 0; i < MAX_BORU; i++) {
            if (borular[i].aktif) {
                // Üst Boru (Ters Çevrilmiş)
                DrawTexturePro(boruDoku, 
                    (Rectangle){ 0, 0, boruDoku.width, -boruDoku.height }, // Y ekseninde -height ile ters çevir
                    borular[i].ust, 
                    (Vector2){ 0, 0 }, 
                    0.0f, WHITE);

                // Alt Boru (Normal)
                DrawTexturePro(boruDoku, 
                    (Rectangle){ 0, 0, boruDoku.width, boruDoku.height }, 
                    borular[i].alt, 
                    (Vector2){ 0, 0 }, 
                    0.0f, WHITE);
                    

            }
        }

        // Kuşu Çiz
        DrawCircleV(kus.pozisyon, kus.yaricap, kus.renk);
        DrawCircleLines((int)kus.pozisyon.x, (int)kus.pozisyon.y, kus.yaricap, RAYWHITE); // Beyaz kontür

        // Kar Tanesini Çiz
        for (int i = 0; i < MAX_KAR; i++) {
            DrawCircleV(karlar[i].pozisyon, karlar[i].cap, RAYWHITE);
        }

        // Skoru Çiz
        DrawText(TextFormat("Puan: %02i", skor), 20, 20, 40, BLACK);
        DrawText(TextFormat("En Yuksek Puan: %02i", enYuksekSkor), 20, 60, 20, GRAY);
        
    } else {
        // Oyun bittiğinde düz renk arkaplan
        ClearBackground((Color){ 10, 20, 40, 255 });

        DrawText("OYUN BITTI", ekranGenislik / 2 - MeasureText("OYUN BITTI", 40) / 2, 20, 40, RED);
        DrawText(TextFormat("Final Puani: %i", skor), ekranGenislik / 2 - MeasureText(TextFormat("Final Puani: %i", skor), 20) / 2, 70, 20, WHITE);
        DrawText("Yeniden Baslamak icin ENTER'a basin", ekranGenislik / 2 - MeasureText("Yeniden Baslamak icin ENTER'a basin", 20) / 2, 100, 20, LIGHTGRAY);
        
        // GIF'i Çiz
        DrawTexture(noelBabaDoku, ekranGenislik / 2 - noelBabaDoku.width / 2, 130, WHITE);
    }

    EndDrawing();
}

void OyunuKapat() {
    // Varsa dokuları boşalt
}

void SkoruYukle() {
    FILE *dosya = fopen("highscore.txt", "r");
    if (dosya != NULL) {
        if (fscanf(dosya, "%d", &enYuksekSkor) != 1) {
            enYuksekSkor = 0; // Okuma hatası
        }
        fclose(dosya);
    } else {
        enYuksekSkor = 0; // Dosya henüz yok
    }
}

void SkoruKaydet() {
    FILE *dosya = fopen("highscore.txt", "w");
    if (dosya != NULL) {
        fprintf(dosya, "%d", enYuksekSkor);
        fclose(dosya);
    }
}
