import React, { useState } from 'react';
import { Routes, Route } from 'react-router-dom';
import './main.css';
import SideBar from '../../components/SideBar/SideBarComponent';
import HomePage from '../home/Home';
import characterImg from '../../assets/images/character.png';
import Header from '../../components/Header/Header';
import History from '../history/History';
import Vallets from '../vallets/vallets';
import Metrics from '../metrics/Metrics';
import Queue from '../queue/Queue';

const Main = () => {

    const [active, setActive] = useState(0);

    const changePage = (index) => {
        setActive(index)
    }

    const pages = ['Painel Administrativo', 'Manobristas', 'Histórico', 'Métricas'];

    return (
        <div id="main">
            <SideBar active={active} changePage={changePage} />

            <div className="container">
                <Header name='Elias Biondo' role='Administrador da unidade' img={characterImg} title={pages[active]}/>
                <Routes>
                    <Route path='/app' element={<HomePage />} />
                    <Route path='/vallets' element={<Vallets />} />
                    <Route path='/history' element={<History />} />
                    <Route path='/metrics' element={<Metrics />} />
                    <Route path='/app/queue' element={<Queue />} />
                </Routes>
            </div>
        </div>
    )

}

export default Main