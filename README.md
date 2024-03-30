# Project CollaborativeNotepad

### (Original) description of the app (in Romanian)
CollaborativeNotepad (A) <br>
Sa se realizeze o aplicatie client/server ce permite editarea simultana a fisierelor text. Un fisier va putea fi editat in acelasi timp de maxim doi clienti. Serverul va permite insa mai multe astfel de sesiuni concurente de editare pe documente diferite (ex. 2 perechi de cate 2 clienti pe 2 documente). Serverul va stoca fisierele, punand la dispozitia clientilor operatiile de creare si download. Aplicatia va trebui sa implementeze un protocol de comunicare pentru operarea concurenta asupra fisierelor. Se va avea in vedere consistenta documentului editat la modificari simultane asupra unei parti comune (ex. plasarea cursorului celor doi clienti cand unul scrie si celalalt sterge in aceeasi pozitie).

### About my own app
Collaborative Notepad is an app that allows users to interact with each others (virtual) files and write text in there through a notepad. The app is quite simple but there are some key features like communication through database and encrypted database data, would allow users to recieve/invite someone to edit on a file with them, will provide a download option so that the files can be downloaded on each editor's computer. An account is necessary for gaining access to any of the functions and a singup is necessary at first use. 

### How to use it
You need to make an account first if yours is not already in the data base (since it probably isn't and the passwords are not visible of course, it is really necessary). By the way, the data has to be a certain format (eg. it cant be too short), and the username has to be unique.

Then, you can log in and out of your respective account using login [username] [password] or logout [username] at logout.

The next step would most likely be to create your first document by using the command create_document and provide a name. Please note that if it doesn't work, it is most probably because of the format of your name. This step creates a document with your data attached to it, so you are the only one that has permission to edit the document.

When needing to edit a document, you can just type the respective command and you would be allowed to type your data in there. Please note that the only way to modify the document is to save it explicitly, if you did not save the edit all the typed data will be lost. 

The way to invite someone to edit a document is to use the command for edit and to add the user [username] as an editor for your document. You can't invite two people to collaborate on a document at the same time, since collaboration at the same time with 3 or more people is not allowed in this app.

### ATTENTION
Please note that it can have many bugs/errors since it was not tested enough for each possible use case.

### Notes
This project was developed entirely on a Linux system, using only C, and it also made for Linux systems. It may not work as well on a Windows or Mac.
