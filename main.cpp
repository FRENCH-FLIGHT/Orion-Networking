#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <Shlwapi.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")

// Fonction pour changer l'adresse IP
bool changeIPAddress(const std::string& ipAddress, const std::string& subnetMask, const std::string& defaultGateway)
{
    DWORD adapterIndex = 0; // Index de l'adaptateur réseau

    // Récupération de l'index de l'adaptateur réseau
    IP_ADAPTER_ADDRESSES* pAdapterAddresses = nullptr;
    ULONG outBufLen = 0;
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAdapterAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(new char[outBufLen]);
        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAdapterAddresses, &outBufLen) == NO_ERROR)
        {
            IP_ADAPTER_ADDRESSES* pAdapter = pAdapterAddresses;
            while (pAdapter)
            {
                if (pAdapter->OperStatus == IfOperStatusUp)
                {
                    adapterIndex = pAdapter->IfIndex;
                    break;
                }
                pAdapter = pAdapter->Next;
            }
        }
    }

    delete[] reinterpret_cast<char*>(pAdapterAddresses);

    if (adapterIndex == 0)
    {
        std::cerr << "Aucun adaptateur réseau disponible." << std::endl;
        return false;
    }

    // Configuration de l'adresse IP
    MIB_IPADDRTABLE* pIPAddrTable = nullptr;
    DWORD dwSize = 0;
    if (GetIpAddrTable(pIPAddrTable, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER)
    {
        pIPAddrTable = reinterpret_cast<MIB_IPADDRTABLE*>(new char[dwSize]);
        if (GetIpAddrTable(pIPAddrTable, &dwSize, FALSE) == NO_ERROR)
        {
            for (DWORD i = 0; i < pIPAddrTable->dwNumEntries; ++i)
            {
                if (pIPAddrTable->table[i].dwIndex == adapterIndex)
                {
                    MIB_IPADDRROW* pIPAddrRow = &pIPAddrTable->table[i];

                    // Conversion des adresses IP
                    sockaddr_in newIPAddress, newSubnetMask, newDefaultGateway;
                    inet_pton(AF_INET, ipAddress.c_str(), &(newIPAddress.sin_addr));
                    inet_pton(AF_INET, subnetMask.c_str(), &(newSubnetMask.sin_addr));
                    inet_pton(AF_INET, defaultGateway.c_str(), &(newDefaultGateway.sin_addr));

                    // Configuration des nouvelles adresses IP
                    pIPAddrRow->dwAddr = newIPAddress.sin_addr.S_un.S_addr;
                    pIPAddrRow->dwMask = newSubnetMask.sin_addr.S_un.S_addr;
                    pIPAddrRow->dwBCastAddr = (pIPAddrRow->dwAddr & pIPAddrRow->dwMask) | (~pIPAddrRow->dwMask);
                    pIPAddrRow->dwReasmSize = 0;
                    pIPAddrRow->dwType = MIB_IPADDR_PRIMARY

                    // Libération de la mémoire allouée
                    delete[] reinterpret_cast<char*>(pIPAddrTable);

                    // Affichage du message de succès
                    std::cout << "Adresse IP modifiée avec succès." << std::endl;

                    return true;
                }
            }
        }
    }

    std::cerr << "Échec de modification de l'adresse IP. Code d'erreur : " << GetLastError() << std::endl;
    return false;
}

// Fonction pour vider le cache DNS
bool flushDNSCache()
{
    if (DnsFlushResolverCache() == 0)
    {
        std::cout << "Cache DNS vidé avec succès." << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Échec du vidage du cache DNS. Code d'erreur : " << GetLastError() << std::endl;
        return false;
    }
}

struct DNSInfo
{
    std::string name;
    std::string address;
};

// Fonction pour configurer le DNS
bool configureDNS(const std::string& dnsAddress)
{
    DWORD adapterIndex = 0; // Index de l'adaptateur réseau

    // Récupération de l'index de l'adaptateur réseau
    IP_ADAPTER_ADDRESSES* pAdapterAddresses = nullptr;
    ULONG outBufLen = 0;
    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAdapterAddresses, &outBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        pAdapterAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(new char[outBufLen]);
        if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAdapterAddresses, &outBufLen) == NO_ERROR)
        {
            IP_ADAPTER_ADDRESSES* pAdapter = pAdapterAddresses;
            while (pAdapter)
            {
                if (pAdapter->OperStatus == IfOperStatusUp)
                {
                    adapterIndex = pAdapter->IfIndex;
                    break;
                }
                pAdapter = pAdapter->Next;
            }
        }
    }

    delete[] reinterpret_cast<char*>(pAdapterAddresses);

    if (adapterIndex == 0)
    {
        std::cerr << "Aucun adaptateur réseau disponible." << std::endl;
        return false;
    }

    // Conversion de l'adresse IP DNS
    sockaddr_in dnsAddressStruct;
    inet_pton(AF_INET, dnsAddress.c_str(), &(dnsAddressStruct.sin_addr));

    // Configuration du serveur DNS
    FIXED_INFO fixedInfo;
    memset(&fixedInfo, 0, sizeof(fixedInfo));
    ULONG outBufLenFixedInfo = sizeof(fixedInfo);
    if (GetNetworkParams(&fixedInfo, &outBufLenFixedInfo) == ERROR_BUFFER_OVERFLOW)
    {
        std::cerr << "Erreur lors de la récupération des informations réseau." << std::endl;
        return false;
    }

    // Liste des serveurs DNS préconfigurés avec noms correspondants
    std::vector<DNSInfo> preconfiguredDNS = {
        { "Google DNS", "8.8.8.8" },
        { "Cloudflare DNS", "1.1.1.1" },
        { "OpenDNS", "208.67.222.222" },
        { "Cyberghost", "38.132.106.139." },
        { "Quad9", "9.9.9.9." }
    };

    // Vérifier si l'adresse DNS saisie correspond à un DNS préconfiguré
    auto it = std::find_if(preconfiguredDNS.begin(), preconfiguredDNS.end(), [&](const DNSInfo& info) {
        return info.address == dnsAddress;
    });

    if (it != preconfiguredDNS.end())
    {
        std::cout << "Serveur DNS sélectionné : " << it->name << " (" << it->address << ")" << std::endl;
        fixedInfo.DnsServerList.IpAddress.String = it->address;
        fixedInfo.DnsServerList.IpAddress.IPv4.sin_addr.S_un.S_addr = dnsAddressStruct.sin_addr.S_un.S_addr;
    }
    else
    {
        std::cout << "Adresse DNS non préconfigurée. Utilisation de l'adresse saisie manuellement." << std::
          
// Fonction pour effectuer le spoofing ARP
bool spoofARP(const std::string& targetIP, const std::string& spoofedMAC)
{
    IPAddr targetIPAddress;
    ULONG physicalAddr[2];
    ULONG physicalAddrLen = 6;
    BYTE* pPhysicalAddr = reinterpret_cast<BYTE*>(&physicalAddr[0]);

    // Conversion de l'adresse IP cible
    targetIPAddress = inet_addr(targetIP.c_str());

    // Conversion de l'adresse MAC falsifiée
    sscanf_s(spoofedMAC.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x", &pPhysicalAddr[0], &pPhysicalAddr[1], &pPhysicalAddr[2], &pPhysicalAddr[3], &pPhysicalAddr[4], &pPhysicalAddr[5]);

    // Envoi du paquet ARP falsifié
    DWORD result = SendARP(targetIPAddress, 0, pPhysicalAddr, &physicalAddrLen);

    if (result == NO_ERROR)
    {
        std::cout << "Spoofing ARP effectué avec succès." << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Échec du spoofing ARP. Code d'erreur : " << result << std::endl;
        return false;
    }
}

// Fonction pour bloquer le tracking Internet sur les navigateurs
bool blockTracking()
{
    // Chemin du fichier hosts
    std::string hostsFilePath = "C:\\Windows\\System32\\drivers\\etc\\hosts";

    // Adresse IP locale pour rediriger les requêtes
    std::string localIPAddress = "127.0.0.1";

    // Ligne à ajouter dans le fichier hosts pour bloquer le tracking
    std::string blockingEntry = localIPAddress + " www.google-analytics.com";

    // Vérifier si l'entrée de blocage est déjà présente dans le fichier hosts
    std::ifstream hostsFile(hostsFilePath);
    std::string line;
    while (std::getline(hostsFile, line))
    {
        if (line.find(blockingEntry) != std::string::npos)
        {
            std::cout << "Le blocage du tracking est déjà activé." << std::endl;
            hostsFile.close();
            return true;
        }
    }
    hostsFile.close();

    // Ajouter l'entrée de blocage dans le fichier hosts
    std::ofstream hostsFileOut(hostsFilePath, std::ios::app);
    if (hostsFileOut.is_open())
    {
        hostsFileOut << std::endl;
        hostsFileOut << "# Blocage du tracking Internet" << std::endl;
        hostsFileOut << blockingEntry << std::endl;
        hostsFileOut.close();
        std::cout << "Le blocage du tracking a été activé." << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Impossible d'ouvrir le fichier hosts pour écriture." << std::endl;
        return false;
    }
}

// Fonction pour débloquer le tracking Internet sur les navigateurs
bool unblockTracking()
{
    // Chemin du fichier hosts
    std::string hostsFilePath = "C:\\Windows\\System32\\drivers\\etc\\hosts";

    // Ligne de blocage à rechercher et supprimer
    std::string blockingEntry = "127.0.0.1 www.google-analytics.com";

    // Vérifier si l'entrée de blocage est présente dans le fichier hosts
    std::ifstream hostsFile(hostsFilePath);
    std::string line;
    bool foundBlockingEntry = false;
    std::ofstream tempFile("temp_hosts.txt");
    while (std::getline(hostsFile, line))
    {
        if (line.find(blockingEntry) == std::string::npos)
        {
            tempFile << line << std::endl;
        }
        else
        {
            foundBlockingEntry = true;
        }
    }
    hostsFile.close();
    tempFile.close();

    // Supprimer le fichier hosts original
    if (remove(hostsFilePath.c_str()) != 0)
    {
        std::cerr << "Impossible de supprimer le fichier hosts." << std::endl;
        return false;
    }
    
    // Renommer le fichier temporaire en fichier hosts
    if (rename("temp_hosts.txt", hostsFilePath.c_str()) != 0)
    {
        std::cerr << "Impossible de renommer le fichier temporaire." << std::endl;
        return false;
    }

    if (foundBlockingEntry)
    {
        std::cout << "Le blocage du tracking a été désactivé." << std::endl;
        return true;
    }
    else
    {
        std::cout << "Le blocage du tracking n'était pas activé." << std::endl;
        return false;
    }
}

void spoofing()
{
    // Afficher un message pour indiquer le début du spoofing
    std::cout << "Démarrage du spoofing..." << std::endl;

    // Demander les informations nécessaires à l'utilisateur
    std::string targetIP;
    std::string spoofedMAC;

    std::cout << "Adresse IP cible : ";
    std::cin >> targetIP;

    std::cout << "Adresse MAC falsifiée : ";
    std::cin >> spoofedMAC;

    // Appeler la fonction de spoofing ARP
    bool success = spoofARP(targetIP, spoofedMAC);
    if (success)
    {
        std::cout << "Spoofing ARP effectué avec succès." << std::endl;

        // Demander à l'utilisateur s'il souhaite bloquer ou débloquer le tracking
        char choice;
        std::cout << "Voulez-vous bloquer (B) ou débloquer (D) le tracking Internet sur les navigateurs ? (B/D) : ";
        std::cin >> choice;

        if (choice == 'B' || choice == 'b')
        {
            // Appeler la fonction pour bloquer le tracking
            bool blockSuccess = blockTracking();
            if (blockSuccess)
            {
                std::cout << "Le blocage du tracking a été activé." << std::endl;
            }
            else
            {
                std::cerr << "Échec du blocage du tracking." << std::endl;
            }
        }
        else if (choice == 'D' || choice == 'd')
        {
            // Appeler la fonction pour débloquer le tracking
            bool unblockSuccess = unblockTracking();
            if (unblockSuccess)
            {
                std::cout << "Le blocage du tracking a été désactivé." << std::endl;
            }
            else
            {
                std::cerr << "Échec du déblocage du tracking." << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Échec du spoofing ARP." << std::endl;
    }

    // Afficher un message pour indiquer la fin du spoofing
    std::cout << "Fin du spoofing." << std::endl;
}
      
void displayMenu()
{
    std::cout << "╔════════════════════════════╗" << std::endl;
    std::cout << "║           MENU             ║" << std::endl;
    std::cout << "╠════════════════════════════╣" << std::endl;
    std::cout << "║ 1. Changer l'adresse IP    ║" << std::endl;
    std::cout << "║ 2. Vider les caches réseau ║" << std::endl;
    std::cout << "║ 3. Configurer le DNS       ║" << std::endl;
    std::cout << "║ 4. Activer le spoofer      ║" << std::endl;
    std::cout << "║ 5. Quitter                 ║" << std::endl;
    std::cout << "╚════════════════════════════╝" << std::endl;
    std::cout << "Fais ton choix : ";
}

void showLogo()
{
    std::cout << " ██████╗ ██████╗ ██╗ ██████╗ ███╗   ██╗    ███╗   ██╗███████╗████████╗██╗    ██╗ ██████╗ ██████╗ ██╗  ██╗██╗███╗   ██╗ ██████╗ " << std::endl;
    std::cout << "██╔═══██╗██╔══██╗██║██╔═══██╗████╗  ██║    ████╗  ██║██╔════╝╚══██╔══╝██║    ██║██╔═══██╗██╔══██╗██║ ██╔╝██║████╗  ██║██╔════╝ " << std::endl;
    std::cout << "██║   ██║██████╔╝██║██║   ██║██╔██╗ ██║    ██╔██╗ ██║█████╗     ██║   ██║ █╗ ██║██║   ██║██████╔╝█████╔╝ ██║██╔██╗ ██║██║  ███╗" << std::endl;
    std::cout << "██║   ██║██╔══██╗██║██║   ██║██║╚██╗██║    ██║╚██╗██║██╔══╝     ██║   ██║███╗██║██║   ██║██╔══██╗██╔═██╗ ██║██║╚██╗██║██║   ██║" << std::endl;
    std::cout << "╚██████╔╝██║  ██║██║╚██████╔╝██║ ╚████║    ██║ ╚████║███████╗   ██║   ╚███╔███╔╝╚██████╔╝██║  ██║██║  ██╗██║██║ ╚████║╚██████╔ " << std::endl;
}

int main()
{
    // Ouvrir une fenêtre CMD
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);

    // Afficher le logo
    showLogo();

    // Attendre une entrée de l'utilisateur pour continuer
    std::cout << "Appuie sur une touche pour continuer...";
    std::cin.ignore();
    std::cin.get();

    // Effacer l'écran
    system("cls");

    // Afficher le menu
    displayMenu();

    // Lire le choix de l'utilisateur
    int choice;
    std::cin >> choice;

    // Traiter le choix de l'utilisateur
    while (choice != 5)
    {
        // Effacer l'écran
        system("cls");

        switch (choice)
        {
            case 1:
                std::cout << "En train de changer ton adresse IP... Attends un instant !" << std::endl;
                changeIPAddress(); // Appel de la fonction pour changer l'adresse IP
                std::cout << "Tada ! Ton adresse IP a été changée avec succès !" << std::endl;
                std::cout << "Félicitations ! Tu peux maintenant prétendre être quelqu'un d'autre sur le réseau." << std::endl;
                break;

            case 2:
                std::cout << "En train de nettoyer les petits bouts de données dans les caches réseau... Patience !" << std::endl;
                flushDNSCache(); // Appel de la fonction pour vider les caches réseau
                std::cout << "Voilà ! Les caches réseau ont été vidés avec succès !" << std::endl;
                std::cout << "C'est comme si tu avais balayé les miettes de ton passage sur le réseau. Tout est propre maintenant !" << std::endl;
                break;

            case 3:
                std::cout << "Configurer le DNS en cours..." << std::endl;
                std::cout << "Nous ouvrons actuellement la bibliothèque... Patience !" << std::endl;
                openDNSLibrary(); // Appel de la fonction pour ouvrir la bibliothèque DNS
                std::cout << "Dictionnaire trouvé !" << std::endl;
    
                configureDNS(); // Appel de la fonction pour configurer le DNS

                std::cout << "DNS modifié avec succès !" << std::endl;
                std::cout << "À présent, bonne navigation !" << std::endl;
                break;

            case 4:
                std::cout << "En train de devenir mystérieux... Chuuut !" << std::endl;
                spoofing(); // Appel de la fonction pour activer le mode anonyme
                std::cout << "Abracadabra ! Le mode anonyme est activé !" << std::endl;
                std::cout << "Maintenant, tu es comme un ninja invisible sur le réseau. Fais attention à ne pas te cogner aux serveurs !" << std::endl;
                break;

            default:
                std::cout << "Choix invalide. Veuillez réessayer." << std::endl;
                break;
        }

        // Attendre une entrée de l'utilisateur pour continuer
        std::cout << "Appuie sur une touche pour continuer...";
        std::cin.ignore();
        std::cin.get();

        // Effacer l'écran
        system("cls");

        // Afficher le menu
        displayMenu();

        // Lire le nouveau choix de l'utilisateur
        std::cin >> choice;
    }

    // Attendre une entrée de l'utilisateur pour continuer
        std::cout << "Appuyez sur une touche pour continuer...";
        std::cin.ignore();
        std::cin.get();

        // Effacer l'écran
        system("cls");

        // Afficher le menu
        displayMenu();

        // Lire le nouveau choix de l'utilisateur
        std::cin >> choice;
    }

    // Fermer la fenêtre CMD
    std::cout << "Appuyez sur une touche pour quitter...";
    std::cin.ignore();
    std::cin.get();
    FreeConsole();

    return 0;
}
