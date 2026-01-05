#include <stdio.h>
#include "cobra.h"

int main() {
    printf("=== CobraGL Math Test ===\n\n");

    // 1. Creazione vettori (inizializzazione stile struct x,y,z)
    cobra_vec3 v1 = {{1.0f, 2.0f, 3.0f}};
    cobra_vec3 v2 = {{4.0f, 5.0f, 6.0f}};

    // La stampa usa internamente .comp[], testando la union
    printf("Vettore 1: "); cobra_vec3_print(v1);
    printf("Vettore 2: "); cobra_vec3_print(v2);

    // 2. Operazioni base
    cobra_vec3 sum = cobra_vec3_add(v1, v2);
    printf("Somma (v1 + v2): "); cobra_vec3_print(sum);

    cobra_vec3 sub = cobra_vec3_sub(v2, v1);
    printf("Sottrazione (v2 - v1): "); cobra_vec3_print(sub);

    cobra_vec3 scaled = cobra_vec3_scale(v1, 2.0f);
    printf("Scala (v1 * 2): "); cobra_vec3_print(scaled);

    // 3. Prodotti e Lunghezza
    c_float dot = cobra_vec3_dot(v1, v2);
    printf("Dot Product (v1 . v2): %f\n", dot);

    c_float len = cobra_vec3_length(v1);
    printf("Lunghezza v1: %f\n", len);

    len = cobra_vec3_length(v2);
    printf("Lunghezza v2: %f\n", len);

    // 4. Normalizzazione
    cobra_vec3 norm = cobra_vec3_normalize(v1);
    printf("Normalizzato v1: "); cobra_vec3_print(norm);
    printf("Lunghezza Normalizzato: %f (dovrebbe essere 1.0)\n", cobra_vec3_length(norm));

    norm = cobra_vec3_normalize(v2);
    printf("Normalizzato v2: "); cobra_vec3_print(norm);
    printf("Lunghezza Normalizzato: %f (dovrebbe essere 1.0)\n", cobra_vec3_length(norm));

    // 5. Geometria: Collinearità e Piani
    printf("\n--- Test Geometria ---\n");
    cobra_vec3 p1 = {{0.0f, 0.0f, 0.0f}};
    cobra_vec3 p2 = {{1.0f, 0.0f, 0.0f}};
    cobra_vec3 p3 = {{2.0f, 0.0f, 0.0f}}; // Allineato con p1 e p2
    cobra_vec3 p4 = {{0.0f, 1.0f, 0.0f}}; // Non allineato (forma una L)
    printf("Vettore p1: "); cobra_vec3_print(p1);
    printf("Vettore p2: "); cobra_vec3_print(p2);
    printf("Vettore p3: "); cobra_vec3_print(p3);
    printf("Vettore p4: "); cobra_vec3_print(p4);

   // 5.1 Collinearità
    printf("\n--- Test Collinearità ---\"\n");

    if (cobra_vec3_is_collinear(p1, p2, p3)) {
        printf("p1, p2, p3 sono allineati (Corretto)\n");
    } else {
        printf("ERRORE: p1, p2, p3 dovrebbero essere allineati\n");
    }

    if (!cobra_vec3_is_collinear(p1, p2, p4)) {
        printf("p1, p2, p4 NON sono allineati. Calcolo normale piano...\n");
        cobra_vec3 normal = cobra_vec3_face_normal(p1, p2, p4);
        printf("Normale del piano (p1->p2->p4): "); cobra_vec3_print(normal);
        // p1(0,0,0) -> p2(1,0,0) (X) -> p4(0,1,0) (Y). Cross(X, Y) = Z (0,0,1)
    }

    // 6. Test Grafico (SDL3)
    printf("\n--- Avvio Sistema Grafico ---\n");
    cobra_window window;
    if (cobra_window_create(&window, 800, 600, "CobraGL Engine")) {
        printf("Finestra creata con successo! Premi X per chiudere.\n");
        
        // Definizione di un cubo (8 vertici) centrato nell'origine
        cobra_vec3 cube_vertices[8] = {
            {{-1, -1, -1}}, {{ 1, -1, -1}}, {{ 1,  1, -1}}, {{-1,  1, -1}},
            {{-1, -1,  1}}, {{ 1, -1,  1}}, {{ 1,  1,  1}}, {{-1,  1,  1}}
        };

        // cobra_vec3 cube_vertices[8] = {
        //     {{-1, -1, -1}}, {{ 1, -1, -1}}, {{ 1,  1, -1}}, {{-1,  1, -1}},
        //     {{0, 0,  1}}, {{0, 0,  1}}, {{ 0,  0,  1}}, {{0,  0,  1}}
        // };

        
        // cobra_vec3 cube_vertices[8] = {
        //     {{1, 1, -2}}, {{ -1, 1, -2}}, {{ -1,  -1, -2}}, {{1,  -1, -2}},
        //     {{-1, -1,  2}}, {{ 1, -1,  2}}, {{ 1,  1,  2}}, {{-1,  1,  2}}
        // };

        float angle = DEGREE*5.0f;
        float fov_factor = 300.0f; // Fattore di zoom/FOV
        float camera_dist = 5.0f;  // Distanza della camera dal cubo


       for (int i = 0; i < 8; i++) {
        cube_vertices[i].x += 1.0f;
        cube_vertices[i].y += 0.8f;

        }

        while (!window.should_close) {
            cobra_window_poll_events(&window);
            
            // 1. Pulisci buffer (Sfondo nero: 0xFF000000)
            cobra_window_clear(&window, 0xFF000000);

            // Aggiorna rotazione
            angle += 0.005f;

            // 2. Pipeline 3D: Calcolo dei vertici proiettati
            cobra_vec3 projected_points[8];
            for (int i = 0; i < 8; i++) {
                cobra_vec3 point = cube_vertices[i];

                // A. Rotazione (su tutti gli assi per effetto 3D completo)
                point = cobra_vec3_rotate_x(point, angle);
                point = cobra_vec3_rotate_y(point, angle * 0.5f);
                point = cobra_vec3_rotate_z(point, angle * 0.2f);

                // B. Traslazione (Spostiamo il cubo davanti alla camera lungo Z)
                point.z += camera_dist;

                // C. Proiezione (3D -> 2D)
                projected_points[i] = cobra_vec3_project(point, fov_factor, 800, 600);
            }

            // 3. Disegno delle linee (Wireframe)
            // Colleghiamo i vertici secondo la struttura del cubo
            for (int i = 0; i < 4; i++) {
                // Faccia frontale (0-1, 1-2, 2-3, 3-0)
                cobra_window_draw_line_aa(&window, projected_points[i].x, projected_points[i].y, 
                                                projected_points[(i+1)%4].x, projected_points[(i+1)%4].y, 0.1f, 0xFF00FFFF, false); // Magenta Spesso (SDF)
                
                // Faccia posteriore (4-5, 5-6, 6-7, 7-4)
                cobra_window_draw_line_aa(&window, projected_points[i+4].x, projected_points[i+4].y, 
                                                projected_points[((i+1)%4)+4].x, projected_points[((i+1)%4)+4].y, 3.0f, 0xFFFFFF00, true); // Giallo Medio (Supersampling)
                
                // Collegamenti tra fronte e retro (0-4, 1-5, 2-6, 3-7)
                cobra_window_draw_line(&window, projected_points[i].x, projected_points[i].y, 
                                                projected_points[i+4].x, projected_points[i+4].y, 0xFFFFFFFF); // Bianco Sottile
            }

            // 4. Swap buffers
            cobra_window_present(&window);
        }

        cobra_window_destroy(&window);
    } else {
        printf("Impossibile creare la finestra.\n");
    }

    return 0;
}