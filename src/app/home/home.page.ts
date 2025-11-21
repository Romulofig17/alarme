import { Component, OnInit, NgZone } from '@angular/core';
import { NgClass } from '@angular/common';
import { IonContent, IonModal } from '@ionic/angular/standalone';

import { ref, onValue, set, onChildAdded, query, orderByChild, startAfter } from 'firebase/database';
import { db } from '../../firebase';

@Component({
  selector: 'app-home',
  templateUrl: 'home.page.html',
  styleUrls: ['home.page.scss'],
  imports: [NgClass, IonContent, IonModal],
})

export class HomePage implements OnInit {
  movimentoDetectado: boolean = false;
  ultimoMovimento: number | null = null;
  historico: any[] = [];
  historicoRecente: any[] = [];
  alarmeAtivo: boolean = true;
  modalOpen: boolean = false;
  historicoPorDia: { dia: string; itens: any[] }[] = [];

  iconAlarmeAtivo: string = `
    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-6 h-6">
      <path stroke-linecap="round" stroke-linejoin="round" d="M14.857 17.082a23.848 23.848 0 0 0 5.454-1.31A8.967 8.967 0 0 1 18 9.75V9A6 6 0 0 0 6 9v.75a8.967 8.967 0 0 1-2.31 5.772A23.848 23.848 0 0 0 9.143 17.082ZM12 21a3 3 0 0 0 3-3H9a3 3 0 0 0 3 3Z" />
    </svg>`;

  iconAlarmeInativo: string = `
    <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-6 h-6">
      <path stroke-linecap="round" stroke-linejoin="round" d="M14.857 17.082a23.848 23.848 0 0 0 5.454-1.31A8.967 8.967 0 0 1 18 9.75V9A6 6 0 0 0 6 9v.75a8.967 8.967 0 0 1-2.31 5.772A23.848 23.848 0 0 0 9.143 17.082M12 21a3 3 0 0 0 3-3H9a3 3 0 0 0 3 3Z" />
      <path stroke-linecap="round" stroke-linejoin="round" d="m4.5 4.5 15 15" />
    </svg>`;

  constructor(private ngZone: NgZone) { }

  ngOnInit() {
    const movimentosRef = ref(db, 'movimentos');
    const ultimoMovimentoRef = ref(db, 'ultimoMovimento');

    onValue(movimentosRef, (snapshot) => {
      const historicoData = snapshot.val();

      
      // Pega o historico do firebase
      if (historicoData) {
        this.historico = Object.keys(historicoData)
          .map(key => historicoData[key])
          .sort((a: any, b: any) => (b.timestamp || 0) - (a.timestamp || 0));
        this.historicoRecente = this.historico.slice(0, 3);
        this.historicoPorDia = this.agruparPorDia(this.historico);
      } else {
        this.historico = [];
        this.historicoRecente = [];
        this.historicoPorDia = [];
      }
    });

    // Seta o valor de ultimo movimento
    onValue(ultimoMovimentoRef, (snapshot) => {
      const ultimoMovimento = snapshot.val();
      this.ultimoMovimento = ultimoMovimento;

      this.configurarConsulta(movimentosRef);
    });
  }

  configurarConsulta(movimentosRef: any) {
    if (this.ultimoMovimento === null) {
      console.log("Valor de ultimo movimento não carregado.");
      return;
    }


    // Pega movimentos novos (após o ultimoMovimento timestamp)
    const consulta = query(movimentosRef, orderByChild('timestamp'), startAfter(this.ultimoMovimento - 1));


    onChildAdded(consulta, (snapshot) => {
      const movimento = snapshot.val();

      if (movimento && movimento.movimento === true) {
        this.ngZone.run(() => {
          console.log('Novo Movimento Detectado! Ativando alarme no app.');
          this.movimentoDetectado = true;
          this.alarmeAtivo = true;
        });
      }
    });
  }

  desligarAlarme() {
    const comandoRef = ref(db, 'comandos');

    const payload = {
      desativar_alarme: true
    };

    set(comandoRef, payload)
      .then(() => {
        console.log('Comando "desativar_alarme" enviado para o Firebase!');

        
        const ultimoMovimentoRef = ref(db, 'ultimoMovimento');
        set(ultimoMovimentoRef, null)  // ultimoMovimento vira null

        this.ngZone.run(() => {
          this.movimentoDetectado = false;
          this.ultimoMovimento = null;
          this.alarmeAtivo = false;
        });
      })
      .catch((error) => console.error("Erro ao enviar comando:", error));
  }

  timestampParaData(time: any){
    var date = new Date(time * 1000);
    return date.toLocaleString();
  }

  formatarDia(timestamp: number) {
    const date = new Date((timestamp || 0) * 1000);
    return date.toLocaleDateString('pt-BR');
  }

  agruparPorDia(lista: any[]) {
    const grupos = new Map<string, any[]>();
    lista.forEach(item => {
      if (item && item.movimento) {
        const dia = this.formatarDia(item.timestamp);
        const arr = grupos.get(dia) || [];
        arr.push(item);
        grupos.set(dia, arr);
      }
    });
    const resultado = Array.from(grupos.entries())
      .map(([dia, itens]) => ({ dia, itens: itens.sort((a, b) => (b.timestamp || 0) - (a.timestamp || 0)) }))
      .sort((a, b) => {
        const ta = a.itens[0]?.timestamp || 0;
        const tb = b.itens[0]?.timestamp || 0;
        return tb - ta;
      });
    return resultado;
  }

  openModal() { this.modalOpen = true; }
  closeModal() { this.modalOpen = false; }

}
