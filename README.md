# Stéganographie BMP / PNG / JPEG

## I. Installation

1. Clonez ou téléchargez le projet : `git clone <url_du_projet>`
2. Ouvrez le dossier du projet dans Visual Studio.
3. Cliquez sur : `Demarrer :F5`

## II. Exécution

Le programme ouvre une fenêtre principale comportant :
- Un menu Fichier (ouvrir, sauvegarder, quitter)
- Un menu Stéganographie (cacher, extraire, comparer)
- Un menu Affichage (zoom avant, zoom arrière, réinitialiser)

## III. Formats supportés

L’application gère automatiquement les formats suivants :
- BMP
- PNG
- JPEG/JPG

## IV. Procédure : Insérer un message (Stéganographie)

1. Ouvrez une image via : `Fichier → Ouvrir une image...`
2. Choisissez un fichier : `*.bmp/ *.png / *.jpg / *.jpeg`
3. Ouvrez le menu : `Stéganographie → Cacher un message...`
4. Une zone de texte apparaît en bas de la fenêtre. Écrivez le message que vous souhaitez cacher.
5. Cliquez sur : `Cacher le message`
6. Le message est alors intégré dans les bits LSB de l’image.
7. Sauvegardez votre image : `Fichier → Sauvegarder l’image...` (bien préciser le formet de l'image)

## V. Procédure : Extraire un message

La fonction compare deux images pixel par pixel :
1. Menu : `Stéganographie → Comparer deux images...`
2. Choisissez l’image A puis l’image B.
3. L’application :
   - affiche le nombre de pixels modifiés
   - crée une image de différence : Noir = pixel identique / Rouge = pixel différent
4. Vous pouvez sauvegarder cette image (BMP/PNG/JPG).
